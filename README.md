# Car Control Stable

这是小车控制系统的稳定版独立仓库。它以 `car_control_system` 的 Python 版能力为基准，把能进入交付链路的功能下沉到 C++ 核心和轻量工具里。

## 现在包含什么

- C++ 固定周期控制核心
- mock 电机客户端
- 达妙硬件客户端
- `mode0` / `mode1` / `mode2`
- `neutral` / `demo` / Linux `gamepad` / UDP `remote` / `hybrid`
- 输入平滑、摇杆曲线、驱动速度斜坡
- 转向锁、前进/倒车方向锁
- 启动参数预设 `conservative` / `normal` / `sport`
- 按轮位角色映射电机 ID
- JSONL 遥测
- Python 只读 API
- 静态诊断前端
- 校准入口 `verify` / `probe` / `steer-zero` / `drive-direction`

## 文档入口

- 使用教程：`docs/????.md`
- 参数调节说明：`docs/????.md` 的“常用参数怎么调”
- 控制方式说明：`docs/??????.md`
- 现场检查清单：`docs/??????.md`
- 架构说明：`docs/????.md`
- 后续扩展机械臂、升降台：`docs/??????.md`

## 真机启动

真机常用命令是一行：

```bash
./scripts/start_car.sh
```

默认使用：

- 本机手柄：`CAR_INPUT=gamepad`
- 低速安全模式：`CAR_MODE=mode2`
- 保守启动参数预设：`CONTROL_PROFILE=conservative`

远程控制：

```bash
CAR_INPUT=remote ./scripts/start_car.sh
```

混合控制：

```bash
CAR_INPUT=hybrid ./scripts/start_car.sh
```

详细控制方式看：

```text
docs/??????.md
```

## Windows 本地 mock

Windows 只建议跑 mock：

```powershell
.\scripts\check_and_start.ps1
```

只做配置检查：

```powershell
python .\tools_python\config_check.py --target mock --input demo
```

打开诊断页面：

```text
http://127.0.0.1:8765/
```

## 远程输入联调

板端或 mock 核心监听远程输入：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
```

电脑端发送 demo：

```powershell
.\scripts\send_remote_demo.ps1
```

## 校准

```powershell
.\scripts\calibrate_mock.ps1
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes
```

## 说明

- 这个仓库不修改原始 `Car` 和 `car_control_system`
- Python 版仍然更适合快速试验和写测试
- stable 版更适合交付、现场运行和真机前检查
- 真机硬件还需要在 ARM Ubuntu + USB-CANFD 环境确认
- 后续如果加机械臂、升降台，建议放到上一级 `robot_platform/arm`、`robot_platform/lift`，不要直接塞进底盘控制循环
