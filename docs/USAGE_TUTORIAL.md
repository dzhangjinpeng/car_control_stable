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

## 5. 常用参数怎么调

底盘参数主要在这几个文件里：

```text
configs/control.json            基础控制参数
configs/control_profiles.json   启动参数预设
configs/safety.json             安全限幅和远程超时
configs/input.json              手柄轴、按钮和输入死区
configs/hardware.json           电机 ID、轮位映射、反向表
```

改完参数后先跑配置检查：

```bash
python3 tools_python/config_check.py --target hardware --input gamepad --profile conservative
```

Windows mock 可以这样检查：

```powershell
python .\tools_python\config_check.py --target mock --input demo --profile conservative
```

### 5.1 最大速度

基础最大速度在 `configs/control.json`：

```json
"max_linear_speed_mps": 0.5
```

单位是 `m/s`，表示底盘线速度上限。第一次真机测试建议低一点，比如 `0.2` 到 `0.35`。确认电机方向、转向零位和急停都可靠后，再逐步提高。

启动参数预设也会覆盖最大速度，在 `configs/control_profiles.json`：

```json
"conservative": {
  "max_linear_speed_mps": 0.35
}
```

启动时选择：

```bash
CONTROL_PROFILE=conservative ./scripts/start_car.sh
CONTROL_PROFILE=normal ./scripts/start_car.sh
CONTROL_PROFILE=sport ./scripts/start_car.sh
```

现场建议顺序：

```text
conservative -> normal -> sport
```

不要第一次上车就用 `sport`。

### 5.2 mode2 低速比例

`mode2` 是低速安全调试模式。它不会直接用满 `max_linear_speed_mps`，还会乘一个比例：

```json
"mode2_speed_scale": 0.25
```

如果 `max_linear_speed_mps=0.5`，`mode2_speed_scale=0.25`，那么 mode2 的速度大约是 `0.125 m/s`。

第一次真机建议：

```json
"mode2_speed_scale": 0.15
```

车轮方向、手柄方向、急停都确认后，再回到 `0.25`。

### 5.3 最大转向角

基础最大转向角在 `configs/control.json`：

```json
"max_steering_degrees": 35.0
```

`mode2` 还有单独限制：

```json
"mode2_max_steering_degrees": 15.0
```

如果转向机构还没完全确认，不要先调大这个值。先用 `mode2_max_steering_degrees=10.0` 到 `15.0` 做低速测试，确认不会顶机械限位，再逐步放大。

### 5.4 驱动速度斜坡

驱动速度斜坡在 `configs/control.json`：

```json
"drive_speed_ramp_mps_per_s": 3.0
```

它控制速度变化有多快。值越大，响应越冲；值越小，起步和刹停越柔。

建议：

- 新车第一次测试：`1.0` 到 `2.0`
- 普通调试：`2.0` 到 `3.0`
- 已确认机械和控制稳定后：再考虑更高

如果出现起步猛、车身抖、轮子打滑，优先降低这个值。

### 5.5 摇杆死区和平滑

输入死区有两个地方：

```json
// configs/control.json
"deadzone": 0.05
```

```json
// configs/input.json
"deadzone": 0.15
```

`input.json` 更偏手柄原始输入过滤，`control.json` 更偏控制计算前的统一处理。手柄松开后如果车还有微小目标速度，先调大 `configs/input.json` 的 `deadzone`，比如从 `0.15` 调到 `0.20`。

平滑参数：

```json
"throttle_smoothing_alpha": 0.18,
"steering_smoothing_alpha": 0.22
```

值越小越稳，但响应更慢；值越大越跟手，但更容易抖。第一次真机不要急着调大。

### 5.6 摇杆曲线

摇杆曲线在 `configs/control.json`：

```json
"throttle_curve_power": 1.2,
"steering_curve_power": 1.4
```

大于 `1.0` 时，小摇杆输入会更细腻，大摇杆输入仍能接近满输出。

建议：

- 低速细调不好控制：把 `steering_curve_power` 提到 `1.6`
- 觉得油门前半段太肉：把 `throttle_curve_power` 降到 `1.0` 到 `1.1`
- 不确定时保持默认

### 5.7 电机安全限幅

安全限幅在 `configs/safety.json`：

```json
"max_drive_motor_speed_rad_s": 3.0,
"max_steer_motor_position_rad": 1.9,
"remote_timeout_s": 0.2
```

含义：

- `max_drive_motor_speed_rad_s`：驱动电机速度硬限制
- `max_steer_motor_position_rad`：转向电机位置硬限制
- `remote_timeout_s`：远程控制多久没收到包就判定超时

这些是保护层参数，不建议为了“跑快”随便放大。速度要优先调 `control.json` 和 `control_profiles.json`，不是先改 `safety.json`。

### 5.8 手柄按钮和轴位

手柄映射在 `configs/input.json`：

```json
"left_x_axis": 0,
"left_y_axis": 1,
"right_x_axis": 3,
"right_y_axis": 4,
"mode_button": 1,
"steering_lock_button": 4,
"drive_direction_button": 5,
"emergency_stop_button": 6
```

如果手柄方向反了或按钮不对，先不要改控制算法。应该先确认板端 `/dev/input/js0` 的轴和按钮编号，再改这个文件。

### 5.9 电机 ID、轮位和反向

硬件映射在 `configs/hardware.json`。最重要的是：

- `motor_ids`：所有电机 ID
- `drive_motor_ids`：驱动电机 ID
- `steer_motor_ids`：转向电机 ID
- `drive_motor_roles`：驱动轮位到电机 ID 的映射
- `steer_motor_roles`：转向轮位到电机 ID 的映射
- `inverted_drive_motor_ids`：需要反向的驱动电机 ID

如果某个轮子方向反了，不要改最大速度，也不要改手柄输入。先架空车轮，跑 `drive-direction`，然后改 `inverted_drive_motor_ids`。

### 5.10 推荐调参顺序

第一次真机建议按这个顺序：

```text
1. conservative profile
2. mode2
3. 车轮架空
4. 确认急停
5. 确认电机 ID 和轮位
6. 确认驱动方向
7. 确认转向零位
8. 调手柄死区
9. 调最大速度
10. 调速度斜坡
11. 调转向角
12. 再考虑 normal / sport
```

每次只改一个参数。改完先跑配置检查，再跑 mock 或架空真机，不要一次改很多项再上地测试。

## 6. 真机本机手柄控制

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

## 7. 真机远程控制

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

## 8. 打开诊断页面

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

## 9. 校准流程

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

## 10. 常见问题定位

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
