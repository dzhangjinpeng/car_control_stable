# 交付说明

## 当前交付物

```text
car_control_stable/
  configs/       运行配置和控制档位
  core_cpp/      C++ 控制核心
  docs/          架构和现场说明
  frontend/      只读诊断前端
  scripts/       构建、运行、校准脚本
  tools_python/  API 和远程输入工具
```

## 相比 Python 版已经对齐的功能

- 输入平滑
- 驱动速度斜坡
- 转向锁
- 前进/倒车方向锁
- 控制档位
- 轮位角色映射
- UDP remote / hybrid
- 校准入口
- 配置健康检查
- 前端诊断字段

## 本地验证顺序

```powershell
.\scripts\check_and_start.ps1 -Target mock -InputMode demo -ControlProfile conservative
.\core_cpp\build.ps1
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file logs\mock_telemetry.jsonl
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --control-profile conservative --max-loops 1000 --telemetry-file logs\profile_telemetry.jsonl
.\scripts\run_api.ps1
.\scripts\send_remote_demo.ps1
```

只检查配置：

```powershell
python .\tools_python\config_check.py --target mock --input demo --control-profile conservative
```

## 真机前检查

- 车轮架空
- 急停可用
- 电机 ID 和轮位映射确认过
- `inverted_drive_motor_ids` 已通过低速点动确认
- `/dev/input/js0` 或手柄设备路径正确
- 达妙串口和 CANFD 连接正常

板端一键检查并启动：

```bash
TARGET=hardware CAR_INPUT=gamepad CONTROL_PROFILE=conservative ./scripts/check_and_start.sh
```

## 校准流程

```powershell
.\build\manual\car_control_core.exe --mock --calibrate verify --report-file logs\calibration.json
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes --save-flash
```
