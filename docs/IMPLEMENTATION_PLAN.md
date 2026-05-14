# 实现计划

## 阶段 0：只做架构

当前阶段只创建目录和文档，不迁移代码。

完成标准：

- 独立目录存在。
- 不修改 `../Car`。
- 不修改 `../car_control_system`。
- 架构文档写清楚职责边界。

## 阶段 1：C++ 最小可跑骨架

目标：

- 创建 `core_cpp` CMake 项目。
- 实现 `main.cpp`、配置加载、mock 电机、固定周期循环。
- 不连接真实硬件。

验收：

```text
car_control_core --mock --max-loops 1000
```

输出：

- 平均循环耗时。
- 最大循环耗时。
- JSONL 遥测。

## 阶段 2：安全和 mode2

目标：

- 实现 `SafetyManager`。
- 实现 `Mode2SafeDebug`。
- 支持急停、限速、转向限幅、退出停车。

验收：

- 急停输入能让驱动速度归零。
- 超出限幅的命令会被截断。
- 模式切换时驱动先停。

## 阶段 3：接入达妙硬件

目标：

- 从原始 C++ 迁移达妙硬件接口。
- 封装成 `DamiaoMotorClient`。
- 保留 `MockMotorClient`。

验收：

- 能读取电机 position / velocity。
- 能 enable / disable。
- 能发低速速度命令。
- 能发转向位置命令。
- 正常运行不写零点。

## 阶段 4：迁移 mode0 和 mode1

目标：

- mode0 保留原地旋转到位后再驱动。
- mode1 使用阿克曼转向。
- 配置参数从文件读取。

验收：

- mode0 未到位时驱动轮为 0。
- mode1 输入前进时四轮速度合理。
- mode1 转向时内外轮角度和速度合理。

## 阶段 5：诊断和工具

目标：

- Python 工具读取 JSONL。
- 前端读取只读 API。
- 现场检查脚本统一入口。

验收：

- 可以看到 input / target / actual / error。
- 可以定位输入、控制、硬件、机械层问题。

