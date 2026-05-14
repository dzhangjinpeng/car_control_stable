# Car Control Stable

这是小车控制系统的第三套独立方案，目标是把正式跑车需要的实时控制下沉到 C++，同时保留 Python 和前端在调试、配置、校准、诊断上的效率。

这个目录不会修改现有两个项目：

- `../Car`：原始 C++ 达妙电机例程。
- `../car_control_system`：当前 Python + C++ 桥接 + 前端诊断版本。

## 目标

- C++ 负责高频控制循环、硬件通信、急停和电机命令下发。
- Python 负责配置生成、校准流程、日志分析、远程输入工具和现场脚本。
- 前端负责只读诊断，不直接控制电机。
- 所有危险动作必须有明确的人工确认或独立工具入口，正常启动不能自动写电机零点。

## 当前阶段

当前完成了可交付的第一版稳定核心。Windows 上可以直接跑 mock；ARM Ubuntu 板端可以编译带达妙 USB-CANFD 的硬件版本。

优先阅读：

- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/DELIVERY.md`
- `docs/FIELD_CHECKLIST.md`

## 第一版运行方式

在 Windows / Linux 上先编译 C++ mock 核心：

```bash
cmake -S core_cpp -B build/core_cpp
cmake --build build/core_cpp
```

如果 Windows 上的 MinGW/CMake 因中文路径构建失败，可以用项目内的 PowerShell 编译脚本：

```powershell
.\core_cpp\build.ps1
```

运行 500Hz mock 循环：

```bash
./build/core_cpp/car_control_core --mock --input demo --max-loops 1000 --telemetry-file telemetry.jsonl
```

Windows PowerShell 下可执行文件一般是：

```powershell
.\build\core_cpp\car_control_core.exe --mock --input demo --max-loops 1000 --telemetry-file telemetry.jsonl
```

如果使用 `build.ps1` 编译，运行：

```powershell
.\build\manual\car_control_core.exe --mock --input demo --max-loops 1000 --telemetry-file telemetry.jsonl
```

Windows 直接编译出来的版本只适合 mock。板端硬件版本需要用 `scripts/build_board.sh` 显式开启达妙硬件支持。

## 当前控制核心能做什么

- 读取 `configs/control.json`、`configs/hardware.json`、`configs/safety.json`。
- 以 `loop_period_s = 0.002` 运行固定周期循环。
- 使用 `MockMotorClient` 模拟电机。
- 支持 `neutral`、`demo` 和 Linux `gamepad` 输入。
- 支持 `mode0` 原地旋转、`mode1` 阿克曼转向、`mode2` 低速安全调试。
- 支持 mock 电机和可选达妙硬件电机客户端。
- 生成驱动轮速度目标和转向电机位置目标。
- 执行急停输入归零。
- 执行速度和转向目标限幅。
- 输出 JSONL 遥测。
- 提供只读 Python 遥测 API 和静态诊断前端。

## 当前还不能做什么

- 还没有迁移校准工具。
- 硬件路径需要在 ARM Ubuntu + USB-CANFD + 达妙电机环境实测。

## 诊断页面

先运行核心生成遥测：

```powershell
.\scripts\run_mock.ps1
```

再开一个终端启动 API：

```powershell
.\scripts\run_api.ps1
```

浏览器打开：

```text
http://127.0.0.1:8765/
```
