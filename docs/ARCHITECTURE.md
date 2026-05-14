# 稳定版小车控制系统架构

## 1. 设计目标

这个版本不是简单重写现有 Python 项目，也不是直接复制原始 C++ 项目。它的目标是把两边的优点合起来：

- 原始 C++ 的优势：硬件链路短、实时性更稳定、适合 500Hz 控制循环。
- 现有 Python 项目的优势：配置清晰、日志充分、前端可诊断、校准和 mock 更方便。

最终形态：

```text
正式控车：C++ 实时核心
现场工具：Python 脚本
诊断界面：Web 前端
配置入口：JSON / YAML 配置文件
```

## 2. 顶层目录

```text
car_control_stable/
  core_cpp/          正式控车核心，负责实时循环和硬件通信
  tools_python/      校准、配置检查、日志分析、远程输入发送等工具
  frontend/          只读诊断前端
  configs/           车辆、控制、输入、网络、安全配置
  protocols/         C++ 核心、Python 工具、前端之间的数据协议定义
  docs/              架构、实现计划、现场流程
```

## 3. 运行时分层

```text
输入层
  本地手柄 / 远程 UDP / 调试脚本

控制层
  模式状态机
  输入过滤
  运动学计算
  速度斜坡

安全层
  急停
  通信超时
  目标限幅
  转向限位
  启动检查
  退出停车

硬件层
  达妙电机控制
  USB-CANFD
  libu2canfd.a

诊断层
  遥测 ring buffer
  JSONL 日志
  只读 HTTP / WebSocket
  前端页面
```

## 4. 进程划分

### 4.1 `car_control_core`

C++ 可执行文件，是正式控车主进程。

职责：

- 加载配置。
- 初始化 CANFD 和电机。
- 以固定周期运行控制循环。
- 读取本地手柄或远程输入。
- 执行 mode0 / mode1 / mode2。
- 执行安全策略。
- 下发电机命令。
- 输出遥测。

不做：

- 不自动保存电机零点。
- 不直接提供复杂 Web 页面。
- 不把调试流程写死在主循环里。

### 4.2 `car_control_api`

可选诊断服务，可以先由 Python 实现，后续也可以由 C++ 内置。

职责：

- 读取核心进程遥测。
- 提供只读 API。
- 给前端展示状态。

限制：

- 默认不开放电机控制写接口。
- 如果未来需要写接口，必须单独加权限和安全确认。

### 4.3 `tools_python`

Python 工具集合。

职责：

- 生成和检查配置。
- 硬件校准。
- 手柄轴检测。
- 远程输入发送。
- JSONL 日志分析。
- 打包部署。

这些工具可以频繁改，但不能影响正式核心的实时性。

## 5. 核心控制循环

C++ 核心循环保持固定周期，目标 500Hz：

```text
loop_period = 0.002s

while running:
  loop_start = now
  input = input_source.poll()
  safe_input = safety.filter_input(input)
  command = controller.update(safe_input, state)
  safe_command = safety.filter_command(command)
  motor_client.send(safe_command)
  telemetry.publish(input, command, feedback, safety_state)
  sleep_until(loop_start + loop_period)
```

这个循环必须保持短、清楚、可测。

禁止在循环里做：

- 阻塞网络请求。
- 文件大量写入。
- 前端服务处理。
- 长时间校准动作。
- 复杂动态内存分配。
- 任何需要人工确认的动作。

## 6. 控制模式

### mode0：兼容原地旋转

保留原始 C++ 的旋转能力，但拆成独立模块：

```text
mode0_rotation.cpp
```

关键规则：

- 转向电机先到固定角度。
- 位置到位后才允许驱动轮转。
- 未到位时驱动轮强制 0。

### mode1：正常遥控车模式

正式驾驶主模式：

```text
mode1_ackermann.cpp
```

关键规则：

- 油门控制线速度。
- 转向输入控制前轮阿克曼角。
- 转弯时做速度补偿。
- 大转向时自动降速。

### mode2：低速安全调试

现场首测模式：

```text
mode2_safe_debug.cpp
```

关键规则：

- 限制最大速度。
- 限制最大转向角。
- 用于确认方向、零点、电机 ID 和急停。

## 7. 安全策略

安全层必须独立于具体模式。即使 mode 模块算错了，安全层也要能拦住明显危险命令。

最低安全策略：

- 急停优先级最高。
- 输入超时后目标归零。
- 远程断开后回退本地或停车。
- 启动时驱动轮目标为 0。
- 退出时先停驱动，再禁用电机。
- 转向目标角度限幅。
- 驱动速度限幅。
- 模式切换时先停驱动并回中。
- 正常启动不自动 `set_zero_position`。
- 正常启动不自动 `save_motor_param`。

## 8. 硬件接口

C++ 里定义稳定的最小硬件接口：

```text
MotorClient
  open()
  close()
  enable_all()
  disable_all()
  control_velocity(motor_id, velocity)
  control_position_velocity(motor_id, position, velocity)
  get_position(motor_id)
  get_velocity(motor_id)
  get_torque(motor_id)
```

实现分两种：

```text
DamiaoMotorClient    真硬件
MockMotorClient      本地测试
```

正式控制逻辑只依赖 `MotorClient`，不直接依赖达妙协议细节。

## 9. 配置设计

配置文件拆分：

```text
configs/vehicle.json       车身尺寸、轮半径、轴距、轮距
configs/hardware.json      电机 ID、方向取反、CANFD 序列号、波特率
configs/control.json       速度、死区、平滑、转向角、控制周期
configs/input.json         手柄轴和按钮映射
configs/network.json       远程输入端口、超时
configs/safety.json        急停、限幅、启动检查策略
```

原则：

- 不把现场常改参数写死在 C++。
- C++ 启动时加载配置并校验。
- 配置校验失败时拒绝真机运行。
- mock 模式可以放宽硬件检查。

## 10. 遥测设计

每帧遥测至少包含：

```text
timestamp
loop_index
loop_duration_ms
mode
input_source
link_state
driver_input
safety_state
drive_motor_targets
drive_motor_actuals
steer_motor_targets
steer_motor_actuals
errors
notices
```

用途：

- 控制台显示。
- JSONL 记录。
- 前端 API。
- 现场问题复盘。

判断问题时优先看：

```text
input 有没有
target 有没有
actual 有没有
方向对不对
error 大不大
safety 有没有触发
```

## 11. 数据流

```text
本地手柄
  -> InputSource
  -> InputFilter
  -> Controller
  -> Safety
  -> MotorClient
  -> CANFD
  -> Motor
  -> Feedback
  -> Telemetry

远程手柄
  -> UDP packet
  -> RemoteInputSource
  -> timeout check
  -> same controller path
```

## 12. 与现有两个版本的关系

### 从 `../Car` 继承

- 达妙底层协议。
- `libu2canfd.a` 链接方式。
- 设备序列号读取思路。
- 500Hz C++ 主循环思路。
- mode0 原地旋转关键逻辑。

### 从 `../car_control_system` 继承

- 配置文件思想。
- mock 测试思想。
- mode0 / mode1 / mode2 分层思想。
- 遥测和前端诊断思想。
- 远程输入和 hybrid 回退思想。
- 校准脚本独立化思想。

### 必须避免的旧问题

- 不把所有控制逻辑塞进一个 `main.cpp`。
- 不在正常启动时自动写零点。
- 不把现场参数硬编码。
- 不让前端直接控制电机。
- 不让远程输入没有超时保护。

## 13. 推荐实现顺序

```text
1. C++ 配置结构和校验
2. C++ MockMotorClient
3. C++ 主循环骨架
4. mode2 低速调试
5. 达妙 DamiaoMotorClient
6. 遥测 JSONL
7. mode0 迁移
8. mode1 阿克曼迁移
9. Python 校准工具接入
10. 前端只读 API 接入
11. 现场部署脚本
12. 长时间稳定性测试
```

## 14. 第一版验收标准

第一版不要追求功能全，先追求可控和可验证。

最低验收：

- mock 模式能以 500Hz 跑固定次数。
- 每轮循环有 `loop_duration_ms`。
- 输入、目标、反馈、错误能写入 JSONL。
- 急停能强制 drive target 为 0。
- mode2 能限制速度和转向角。
- 正常启动不会写电机零点。
- 配置错误时拒绝真机启动。

