# Car Control Stable

这是小车控制系统的稳定版独立仓库。

目标很简单：把正式运行需要的控制核心、校准入口、远程输入、只读诊断前端都收进来，形成一个可以单独交付的版本。

## 现在有什么

- C++ 控制核心 `core_cpp`
- mock 电机客户端
- 达妙硬件客户端
- `mode0` / `mode1` / `mode2`
- `neutral` / `demo` / `gamepad` / `remote` / `hybrid`
- 校准入口 `verify` / `probe` / `steer-zero` / `drive-direction`
- JSONL 遥测
- Python 只读 API
- 静态诊断前端
- UDP 远程发送器

## Windows 本地跑法

编译：

```powershell
.\core_cpp\build.ps1
```

运行 mock：

```powershell
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
```

启动诊断 API：

```powershell
.\scripts\run_api.ps1
```

打开浏览器：

```text
http://127.0.0.1:8765/
```

远程输入联调：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
.\scripts\send_remote_demo.ps1
```

## 校准

mock 校准报告：

```powershell
.\scripts\calibrate_mock.ps1
```

驱动方向校准：

```powershell
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
```

转向零点校准：

```powershell
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes
```

## 说明

- 这个仓库不改原始 `Car` 和 `car_control_system`
- 现在可交付的是稳定版架构和工具闭环
- 真机硬件还需要在 ARM Ubuntu + USB-CANFD 环境确认
