# 使用教程

这份文档按实际使用顺序写：先在 Windows 本机确认软件，再到板端真机启动，最后打开诊断页面看状态。

## 1. 先确认你在哪个目录

当前交付版目录是：

```text
F:\公司\小车\car_control_stable
```

不要在 `Car` 和 `car_control_system` 里改交付代码。它们现在只作为原始 C 版和 Python 版参考。

## 2. Windows 本机软件测试

Windows 上不接真实电机，先跑 mock：

```powershell
cd F:\公司\小车\car_control_stable
.\scripts\check_and_start.ps1 -Target mock -InputMode demo
```

通过标准：

- 终端里看到 `CONFIG CHECK PASSED`
- 生成 `logs/mock_telemetry.jsonl`
- 浏览器打开 `http://127.0.0.1:8765/`
- 前端页面能看到模式、输入源、电机目标值、启动检查

如果只想检查配置，不启动核心：

```powershell
python .\tools_python\config_check.py --target mock --input demo
```

## 3. 本机 mock 远程控制测试

开一个终端启动核心：

```powershell
.\build\manual\car_control_core.exe --mock --input remote --mode mode2 --max-loops 0 --telemetry-file logs\remote_telemetry.jsonl
```

再开一个终端发送远程输入：

```powershell
.\scripts\send_remote_demo.ps1
```

诊断页面里重点看：

- `输入链路`：发送时应出现 `remote_online`
- `远程序号`：应持续增加
- 停止发送后：应变成 `remote_timeout`

这一步能证明 UDP 远程控制链路能通，但不能证明真实电机方向正确。

## 4. 真机启动前检查

真机启动前必须先做这些动作：

- 车轮架空
- 急停按钮可用
- USB-CANFD 已插好
- 达妙电机已上电
- `configs/hardware.json` 里的序列号、电机 ID、轮位映射正确
- `configs/input.json` 里的手柄轴和按钮编号正确
- 第一次启动用低速安全参数

真机配置检查：

```bash
python3 tools_python/config_check.py --target hardware --input gamepad --profile conservative
```

## 5. 真机本机手柄控制

手柄插在板端，使用最短命令：

```bash
./scripts/start_car.sh
```

它默认等价于：

```bash
TARGET=hardware CAR_INPUT=gamepad CAR_MODE=mode2 CONTROL_PROFILE=conservative ./scripts/check_and_start.sh
```

含义：

- `CAR_INPUT=gamepad`：板端本机手柄
- `CAR_MODE=mode2`：低速安全调试模式
- `CONTROL_PROFILE=conservative`：启动参数预设，限制速度和动作幅度

注意：`CONTROL_PROFILE` 不是手柄档位。车的实时动作仍然由手柄控制。

## 6. 真机远程控制

板端启动远程输入：

```bash
CAR_INPUT=remote ./scripts/start_car.sh
```

电脑端发送远程输入：

```powershell
python .\tools_python\remote_control_sender.py --host 板端IP --port 23333 --input demo --hz 40
```

如果要本机手柄和远程都可用：

```bash
CAR_INPUT=hybrid ./scripts/start_car.sh
```

`hybrid` 的用途是现场联调：远程包在线时可走远程输入，远程掉线后回到安全状态。

## 7. 打开诊断页面

板端或电脑端启动 API：

```bash
./scripts/run_api.sh
```

Windows 本机：

```powershell
.\scripts\run_api.ps1
```

浏览器访问：

```text
http://127.0.0.1:8765/
```

如果 API 跑在板端，把 `127.0.0.1` 换成板端 IP。

页面里重点看：

- `输入链路`：判断手柄或远程输入是否在线
- `急停`：确认安全状态
- `转向锁`：确认转向是否被锁住
- `方向锁`：确认前进/倒车方向状态
- `驱动电机` 和 `转向电机`：确认目标值、实测值、误差
- `启动检查`：确认配置文件和校准报告状态

## 8. 校准流程

先做只读验证：

```bash
./build/manual/car_control_core --mock --calibrate verify --report-file logs/calibration.json
```

真机上不要一上来就让车轮落地。建议顺序：

```bash
./scripts/calibrate_hardware.sh verify
./scripts/calibrate_hardware.sh probe
./scripts/calibrate_hardware.sh drive-direction
./scripts/calibrate_hardware.sh steer-zero
```

检查重点：

- 每个电机 ID 是否能读到
- 驱动轮正反方向是否和实际一致
- 转向零位是否机械居中
- 校准报告是否写入 `logs/calibration.json`

## 9. 常见问题定位

配置检查失败：

- 看失败项对应哪个 JSON 文件
- 优先检查 `configs/hardware.json` 和 `configs/input.json`

诊断页面打不开：

- 确认 API 是否启动
- 确认端口是 `8765`
- 确认防火墙没有挡住板端 IP

远程控制没反应：

- 确认板端用了 `CAR_INPUT=remote` 或 `CAR_INPUT=hybrid`
- 确认发送端 IP 和端口是板端地址和 `23333`
- 看诊断页面 `输入链路` 是否出现 `remote_online`

手柄没反应：

- 确认手柄在板端能被识别为 `/dev/input/js0`
- 检查 `configs/input.json` 的轴和按钮编号
- 先不要接电机，使用低速模式看遥测输入值是否变化

电机方向反了：

- 先架空车轮
- 用 `drive-direction` 校准
- 修改 `configs/hardware.json` 里的 `inverted_drive_motor_ids`

