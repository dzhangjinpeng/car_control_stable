# 控制方式说明

这里把“本机控制”和“远程控制”分清楚。手柄控制的是车的实时驾驶状态；启动参数预设只是在启动前选择一套安全参数，不是手柄档位。

## 1. 本机手柄控制

本机控制指手柄直接插在板端，C++ 核心从 `/dev/input/js0` 读取输入。

真机常用启动命令：

```bash
./scripts/start_car.sh
```

这个短命令默认等价于：

```bash
TARGET=hardware CAR_INPUT=gamepad CAR_MODE=mode2 CONTROL_PROFILE=conservative ./scripts/check_and_start.sh
```

默认值的意思：

- `TARGET=hardware`：真机硬件模式
- `CAR_INPUT=gamepad`：板端本机手柄
- `CAR_MODE=mode2`：先进入低速安全调试模式
- `CONTROL_PROFILE=conservative`：启动前使用保守参数预设

手柄负责这些实时控制：

- 左摇杆 Y：前进 / 后退
- 左摇杆 X：转向
- `mode_button`：切换 `mode0 / mode1 / mode2`
- `steering_lock_button`：打开或关闭转向锁
- `drive_direction_button`：切换自动 / 只前进 / 只倒车
- `emergency_stop_button`：急停

`CONTROL_PROFILE` 不由手柄切换。它只是开机前的参数预设，用来限制最大速度、低速模式比例、转向时降速比例。

## 2. 远程控制

远程控制指板端只监听 UDP 输入，另一台电脑负责发手柄数据包。

板端启动：

```bash
CAR_INPUT=remote ./scripts/start_car.sh
```

电脑端发送：

```bash
python tools_python/remote_control_sender.py --host <板端IP> --input gamepad --max-seconds 0
```

如果只是测试链路，可以用 demo：

```bash
python tools_python/remote_control_sender.py --host <板端IP> --input demo --max-seconds 5
```

远程链路的判断字段在前端和遥测里都能看到：

- `input_link_state=remote_online`：远程包正在进入
- `input_link_state=remote_hold`：最近一包仍然有效
- `input_link_state=remote_timeout`：远程超时，输入回中位
- `remote_seq`：远程序号
- `remote_latency_s`：最近远程包延迟
- `remote_stale`：远程包是否过期

## 3. 混合控制

混合控制指板端保留本机手柄，同时允许远程接管。

板端启动：

```bash
CAR_INPUT=hybrid ./scripts/start_car.sh
```

规则：

- 本机手柄急停优先级最高
- 远程包有效且 active=true 时，远程接管摇杆输入
- 远程超时或 inactive 时，回退到本机手柄

## 4. Windows 本地 mock

Windows 只建议跑 mock，不直接跑真机硬件。

一键启动 mock：

```powershell
.\scripts\check_and_start.ps1
```

只检查配置：

```powershell
python .\tools_python\config_check.py --target mock --input demo
```

本地远程输入联调：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
.\scripts\send_remote_demo.ps1
```

## 5. 启动参数预设

预设文件是 `configs/control_profiles.json`。

可选值：

- `conservative`：保守，第一次真机默认用这个
- `normal`：正常
- `sport`：更激进，不建议初次真机使用

修改预设不是实时驾驶操作。它只影响程序启动时加载的控制参数。

示例：

```bash
STARTUP_PRESET=normal ./scripts/start_car.sh
```

或者兼容旧变量：

```bash
CONTROL_PROFILE=normal ./scripts/start_car.sh
```
