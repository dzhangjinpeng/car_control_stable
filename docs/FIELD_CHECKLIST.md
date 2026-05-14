# 现场检查清单

## 1. 先跑 mock

```powershell
.\scripts\check_and_start.ps1 -Target mock -InputMode demo -ControlProfile conservative
.\scripts\run_mock.ps1
```

看这些项：

- 程序正常结束
- `logs/mock_telemetry.jsonl` 有内容
- `mode2` 下输入、目标、实测都能写进遥测

## 2. 再开 API

```powershell
.\scripts\run_api.ps1
```

浏览器访问：

```text
http://127.0.0.1:8765/
```

## 3. 校准检查

```powershell
.\scripts\calibrate_mock.ps1
```

通过标准：

- `logs/calibration.json` 生成成功
- 能看到每个电机的 position / velocity / torque

## 4. 驱动方向

```powershell
.\build\manual\car_control_core.exe --mock --calibrate drive-direction --report-file logs\drive_calibration.json
```

通过标准：

- 车轮架空
- 一次只点动一个驱动电机
- 观察轮子正反向后更新 `configs/hardware.json`

## 5. 转向零点

```powershell
.\build\manual\car_control_core.exe --mock --calibrate steer-zero --yes --save-flash
```

通过标准：

- 机械零位已经摆正
- 校准报告生成成功
- 真机时再考虑是否写入 flash

## 6. 远程输入

板端：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
```

电脑端：

```powershell
.\scripts\send_remote_demo.ps1
```

通过标准：

- `input_link_state` 先是 `remote_online`
- 停止发送后变成 `remote_timeout`
- 输入回到中位

## 7. 板端一键启动

```bash
TARGET=hardware CAR_INPUT=gamepad CONTROL_PROFILE=conservative ./scripts/check_and_start.sh
```

通过标准：

- 配置检查全部 PASS
- C++ 核心完成构建
- API 输出诊断地址
- 前端能看到 `startup_checks`
