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

当前完成了第一版 C++ mock 控制核心。它只用于验证主循环、配置、安全限幅和遥测，不连接真实硬件。

优先阅读：

- `docs/ARCHITECTURE.md`
- `docs/IMPLEMENTATION_PLAN.md`

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

当前阶段如果不加 `--mock` 会拒绝启动，这是为了避免还没接入硬件安全检查时误碰真机。

## 当前控制核心能做什么

- 读取 `configs/control.json`、`configs/hardware.json`、`configs/safety.json`。
- 以 `loop_period_s = 0.002` 运行固定周期循环。
- 使用 `MockMotorClient` 模拟电机。
- 支持 `neutral` 和 `demo` 输入。
- 生成驱动轮速度目标和转向电机位置目标。
- 执行急停输入归零。
- 执行速度和转向目标限幅。
- 输出 JSONL 遥测。

## 当前还不能做什么

- 还没有接真实达妙电机。
- 还没有读取真实手柄。
- 还没有实现完整 mode0 / mode1 / mode2。
- 还没有前端 API。
