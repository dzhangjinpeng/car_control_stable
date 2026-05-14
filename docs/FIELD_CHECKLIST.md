# 现场检查清单

## 1. 软件启动

```bash
./scripts/run_mock.sh
```

通过标准：

- 程序完成指定循环。
- 生成 `logs/mock_telemetry.jsonl`。
- demo 后段急停，驱动目标为 0。

## 2. 硬件构建

```bash
./scripts/build_board.sh
```

通过标准：

- 生成 `build/core_cpp/car_control_core`。
- 没有 libusb / libu2canfd 链接错误。

## 2.5 校准报告

mock 验证：

```powershell
.\scripts\calibrate_mock.ps1
```

硬件验证：

```bash
CALIBRATE_ACTION=verify ./scripts/calibrate_hardware.sh
```

通过标准：

- 生成 `logs/calibration.json`。
- 能看到每个电机的 position / velocity / torque。
- 不出现硬件客户端打开失败。

## 3. 真机前检查

- 车轮架空。
- 急停方式明确。
- CANFD 已插好。
- 电机已上电。
- `configs/hardware.json` 的 SN 和电机 ID 正确。
- 手柄设备路径正确。

## 4. 真机 mode2

```bash
CAR_MODE=mode2 ./scripts/run_hardware.sh
```

看遥测：

- `input_source = gamepad`
- `mode = mode2_safe_debug`
- 摇杆动时 target 变化。
- actual 能跟随 target。
- 急停时 drive target 归零。

## 5. 异常定位

```text
输入不变：查手柄和 /dev/input/js0
target 不变：查模式、死区、急停
actual 不变：查 CANFD、电源、电机 ID、波特率
方向反：查 inverted_drive_motor_ids 或轮位映射
转向怪：查零点、轴距、轮距、转向电机角色
```
