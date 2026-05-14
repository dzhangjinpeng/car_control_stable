# 交付说明

## 当前交付物

```text
car_control_stable/
  configs/       运行配置
  core_cpp/      C++ 控制核心
  docs/          架构与现场说明
  frontend/      只读诊断前端
  scripts/       构建、运行、校准脚本
  tools_python/  API 和远程输入工具
```

## 已有能力

- 固定周期控制循环
- mock 电机客户端
- 达妙硬件客户端
- `mode0` / `mode1` / `mode2`
- `neutral` / `demo` / `gamepad` / `remote` / `hybrid`
- JSONL 遥测
- 只读 API
- 前端总览、配置、校准、历史
- 远程控制发送器
- `verify` / `probe` / `steer-zero` / `drive-direction`

## 本地验证顺序

```powershell
.\core_cpp\build.ps1
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
.\scripts\run_api.ps1
.\scripts\send_remote_demo.ps1
```

## 真机前检查

- 车轮架空
- 急停可用
- 电机 ID 和反向列表确认过
- `/dev/input/js0` 或手柄设备路径正确
- 达妙串口和 CANFD 连接正常

## 校准流程

```powershell
.\build\manual\car_control_core.exe --mock --calibrate verify --report-file logs\calibration.json
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes --save-flash
```
