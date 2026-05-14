# 现场检查清单

## 1. Windows 先跑 mock

```powershell
.\scripts\check_and_start.ps1 -Target mock -InputMode demo
```

通过标准：

- 配置检查全部 PASS
- `logs/mock_telemetry.jsonl` 有内容
- 诊断页面能打开

## 2. 真机启动前

- 车轮架空
- 急停可用
- CANFD 已连接
- 电机已上电
- `configs/hardware.json` 的 SN、电机 ID、轮位映射正确
- `configs/input.json` 的手柄轴和按键正确

## 3. 本机手柄控制

```bash
./scripts/start_car.sh
```

通过标准：

- 配置检查全部 PASS
- C++ 核心启动
- API 输出诊断地址
- 前端能看到 `startup_checks`
- 手柄动作能改变 `driver_input`

## 4. 远程控制

板端：

```bash
CAR_INPUT=remote ./scripts/start_car.sh
```

电脑端：

```bash
python tools_python/remote_control_sender.py --host <板端IP> --input gamepad --max-seconds 0
```

通过标准：

- `input_link_state` 先是 `remote_online`
- `remote_seq` 持续增加
- 停止发送后变成 `remote_timeout`
- 输入回到中位

## 5. 混合控制

```bash
CAR_INPUT=hybrid ./scripts/start_car.sh
```

通过标准：

- 本机手柄急停优先
- 远程有效时接管摇杆输入
- 远程超时后回到本机手柄

## 6. 校准

```powershell
.\scripts\calibrate_mock.ps1
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes --save-flash
```

真机写零点前必须确认机械零位已经摆正。
