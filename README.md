# Car Control Stable

这是小车控制系统的稳定版独立仓库。它以 `car_control_system` 的 Python 版能力为基准，把能进入交付链路的功能下沉到 C++ 核心和轻量工具里。

## 对齐后的能力

- C++ 固定周期控制核心
- mock 电机客户端
- 达妙硬件客户端
- `mode0` / `mode1` / `mode2`
- `neutral` / `demo` / Linux `gamepad` / UDP `remote` / `hybrid`
- 输入平滑、摇杆曲线、驱动速度斜坡
- 转向锁、前进/倒车方向锁
- 控制档位 `conservative` / `normal` / `sport`
- 按轮位角色映射电机 ID
- JSONL 遥测
- Python 只读 API
- 静态诊断前端
- 校准入口 `verify` / `probe` / `steer-zero` / `drive-direction`

## Windows 本地跑法

```powershell
.\core_cpp\build.ps1
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
.\scripts\run_api.ps1
```

浏览器打开：

```text
http://127.0.0.1:8765/
```

使用控制档位：

```powershell
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --control-profile conservative --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
```

远程输入联调：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
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
- stable 版现在更适合交付、现场运行和真机前检查
- 真机硬件还需要在 ARM Ubuntu + USB-CANFD 环境确认
