# 后续扩展说明

这个仓库现在交付的是小车底盘控制。后续如果加机械臂、升降台、云台、传感器，不建议把所有代码都继续塞进底盘控制核心里。更稳的做法是把“设备控制”和“整机调度”分层。

## 1. 推荐项目边界

当前仓库职责：

```text
car_control_stable/
  core_cpp/      底盘实时控制
  configs/       底盘配置
  frontend/      底盘诊断页面
  tools_python/  底盘 API 和联调工具
```

后续整车系统建议在上一级新建总目录，例如：

```text
robot_platform/
  chassis/       小车底盘，放 car_control_stable
  arm/           机械臂控制
  lift/          升降台控制
  sensors/       相机、雷达、IMU 等传感器
  supervisor/    整机任务调度
  dashboard/     整机诊断页面
  docs/          总体说明
```

底盘仓库可以作为 `robot_platform/chassis` 使用。这样以后看目录就知道哪个模块负责什么。

## 2. 分层原则

不要让机械臂代码直接改底盘控制循环，也不要让底盘循环直接等待机械臂动作完成。

推荐分层：

```text
任务层 supervisor
  -> 发底盘速度/模式命令
  -> 发机械臂姿态/夹爪命令
  -> 发升降台高度命令

设备层
  -> chassis 底盘控制
  -> arm 机械臂控制
  -> lift 升降台控制

硬件层
  -> CAN / 串口 / USB / GPIO / Ethernet
```

底盘核心只负责“车怎么稳定移动”。机械臂只负责“臂怎么安全到位”。整机调度负责“先移动还是先抬升”。

## 3. 统一通信方式

当前底盘已经支持：

- UDP 远程输入：适合低延迟驾驶控制
- JSONL 遥测：适合记录状态
- Python 只读 API：适合诊断页面读取

后续扩展建议：

- 实时驾驶类命令继续走 UDP
- 机械臂、升降台这类目标位置命令走 HTTP API 或本地消息总线
- 所有模块都输出自己的状态 JSON
- 整机 dashboard 汇总各模块状态

示例状态字段：

```json
{
  "module": "lift",
  "online": true,
  "mode": "hold",
  "target_height_mm": 320,
  "actual_height_mm": 318,
  "fault": null
}
```

## 4. 机械臂模块建议

机械臂不要直接和底盘抢控制权。建议它独立成模块：

```text
arm/
  configs/
    arm.json
    safety.json
  core_cpp/
  tools_python/
  docs/
```

最少要有这些能力：

- 关节 ID 配置
- 关节限位
- 速度/加速度限制
- 急停输入
- 回零或标定流程
- 当前关节角、目标关节角、误差遥测
- 夹爪状态

机械臂启动前检查：

- 机械限位是否可靠
- 回零方向是否正确
- 夹爪开合方向是否正确
- 手臂动作空间是否会撞到底盘或升降台

## 5. 升降台模块建议

升降台独立成模块：

```text
lift/
  configs/
    lift.json
    safety.json
  core_cpp/
  tools_python/
  docs/
```

最少要有这些能力：

- 高度零点
- 上下限位
- 目标高度控制
- 过流或卡滞保护
- 当前高度、目标高度、速度、故障状态遥测

升降台特别要注意互锁：

- 车高速运动时禁止升降
- 升降台未到安全高度时禁止机械臂伸出
- 升降台故障时底盘应进入低速或停车状态

## 6. 整机安全互锁

加机械臂和升降台后，安全逻辑不能只看底盘。整机至少要有这些互锁：

- 全局急停：所有模块立即停
- 底盘低电压：禁止机械臂大动作
- 机械臂伸出：底盘限速
- 升降台升高：底盘限速
- 远程控制掉线：底盘回零，机械臂保持或回安全位
- 任一模块故障：supervisor 标记整机不可自动运行

## 7. 后续代码落点

如果只是继续优化底盘：

```text
car_control_stable/
```

如果开始做整机系统：

```text
robot_platform/
  chassis/car_control_stable/
  arm/
  lift/
  supervisor/
```

不要把机械臂和升降台直接写进 `core_cpp/src/car_controller.cpp`。那会让底盘控制循环越来越难调，也会让一个模块故障影响所有模块。

