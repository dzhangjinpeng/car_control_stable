# 交付说明

## 当前交付内容

这个目录已经是独立项目：

```text
car_control_stable/
  configs/       运行配置
  core_cpp/      C++ 控制核心
  docs/          文档
  scripts/       构建和运行脚本
```

当前可交付能力：

- C++ 固定周期控制核心。
- mock 电机客户端。
- 达妙硬件客户端。
- `mode0` 原地旋转模式。
- `mode1` 阿克曼转向模式。
- `mode2` 低速安全调试模式。
- `neutral` / `demo` / Linux `gamepad` 输入。
- 急停输入归零。
- 驱动速度限幅。
- 转向角限幅。
- JSONL 遥测。
- 只读遥测 API。
- 静态诊断前端。
- Windows mock 编译脚本。
- Linux/ARM 板端硬件构建脚本。

## Windows 本地验证

```powershell
cd F:\公司\小车\car_control_stable
.\core_cpp\build.ps1
.\build\manual\car_control_core.exe --mock --input demo --mode mode2 --max-loops 1000 --telemetry-file telemetry.jsonl
```

也可以一条命令：

```powershell
.\scripts\run_mock.ps1
```

启动诊断页面：

```powershell
.\scripts\run_api.ps1
```

浏览器打开：

```text
http://127.0.0.1:8765/
```

## 打包

```powershell
.\scripts\package_release.ps1
```

输出：

```text
dist/car_control_stable_release.zip
```

## ARM Ubuntu 板端构建

先安装依赖：

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libusb-1.0-0-dev
```

设置 USB-CANFD 权限：

```bash
sudo tee /etc/udev/rules.d/99-usb-canfd.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="34b7", ATTR{idProduct}=="6877", MODE="0666"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger
```

编译硬件版：

```bash
chmod +x scripts/*.sh
./scripts/build_board.sh
```

先跑 mock：

```bash
./scripts/run_mock.sh
```

再跑硬件低速调试模式：

```bash
CAR_MODE=mode2 GAMEPAD_DEVICE=/dev/input/js0 ./scripts/run_hardware.sh
```

## 重要安全说明

这个稳定版的正式控制核心不会在正常启动时自动执行：

```text
set_zero_position
save_motor_param
```

转向零点写入必须以后做成单独校准工具，不能混在主控制程序里。

## 现场第一轮推荐流程

```text
1. Windows 或板端先跑 mock。
2. 板端编译 hardware 版本。
3. 架空车轮。
4. 用 mode2 跑硬件。
5. 先看遥测 input / target / actual。
6. 确认急停有效。
7. 确认驱动方向。
8. 确认转向方向。
9. 再进入 mode1。
10. mode0 最后测试。
```

## 当前限制

- 校准工具还没有迁移进稳定版。
- 硬件版需要在 ARM Ubuntu + USB-CANFD + 达妙电机环境实测。
- Windows 只能可靠跑 mock。
