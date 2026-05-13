# Savebox

基于 ESP32-S3 和 ESP-IDF 的智能储物箱示例项目。

当前工程已经完成从传统单文件示例向 `ESP_APP + ESP_BSP + task` 结构的迁移，具备门锁控制、传感器采集、OLED 显示、UART 命令交互以及 MQTT 联网扩展能力。

## 已实现功能

### 1. 门锁控制

- 使用舵机模拟门锁开关
- 上电默认上锁
- 支持本地函数控制开锁/关锁
- 支持 UART 指令控制开锁/关锁
- 支持 RC522 白名单卡刷卡切换锁状态
- 支持 MQTT 下发 `LOCK` / `UNLOCK` 指令控制锁状态

### 2. RC522 RFID 刷卡识别

- 周期轮询 RC522
- 识别到卡片后打印 UID
- 白名单卡可切换锁状态
- 未知卡会通过串口输出提示
- 内置刷卡冷却时间，避免重复触发

### 3. DHT11 温湿度采集

- 周期读取温度和湿度
- 采样结果保存到运行时状态中心
- 串口输出温湿度结果
- OLED 显示当前温湿度
- MQTT 周期上报温湿度快照

### 4. MQ-2 气体检测

- ADC 采集 MQ-2 模拟量
- 内置预热时间判断
- 使用历史值做增量比较
- 超过阈值判定为气体报警
- 首次报警触发蜂鸣器
- 串口、OLED、MQTT 同步输出状态

### 5. SW180 振动检测

- GPIO 输入读取振动开关状态
- 使用 GPIO 中断捕获振动事件
- 带软件去抖和报警限频
- 振动触发时输出串口日志并驱动蜂鸣器
- OLED 和 MQTT 可看到振动状态

### 6. 蜂鸣器告警

- 采用独立任务和消息队列控制蜂鸣器
- MQ-2 报警时自动鸣叫
- SW180 振动报警时自动鸣叫
- 便于后续扩展更多告警源

### 7. OLED 状态显示

OLED 页面当前显示：

- 锁状态
- 温度
- 湿度
- MQ-2 原始值
- MQ-2 增量值或预热剩余时间
- 振动状态
- 最近识别到的卡号

### 8. UART 命令行交互

串口默认波特率为 `115200`。

当前支持命令：

- `L` 或 `LOCK`：上锁
- `U` 或 `UNLOCK`：开锁
- `S` 或 `STATUS`：输出综合状态
- `DHT?` 或 `DHT11?`：查询温湿度
- `MQ2?`：查询 MQ-2 状态
- `CARD?`：查询最近卡号
- `RC522?` 或 `VERSION?`：查询 RC522 版本寄存器
- `H`、`HELP` 或 `?`：显示帮助

### 9. MQTT 联网能力

项目中已经实现 MQTT 模块，代码位于 `MQTT/` 目录。

当前能力包括：

- Wi-Fi STA 连接
- MQTT Broker 连接
- 在线/离线状态发布
- 周期性上报设备状态快照
- 订阅命令 topic
- 支持命令 `LOCK`、`UNLOCK`、`PING`

注意：
如果 `MQTT/savebox_mqtt_config.h` 中没有填入 Wi-Fi 和 Broker 参数，MQTT 模块会保持禁用状态，不影响其他本地功能运行。

## 软件架构

项目按职责分为 4 层：

### `ESP_BSP`

底层板级支持和外设适配层，负责：

- GPIO
- ADC
- UART
- SPI
- PWM
- OLED 底层初始化
- ESP-IDF 与兼容 HAL 的桥接

### `ESP_APP`

设备功能模块封装，负责：

- 舵机控制
- RC522 驱动封装
- MQ-2 读取封装
- DHT11 读写封装
- SW180 读取封装

### `task`

业务任务层，负责：

- 周期采样
- 状态更新
- 联动报警
- 串口命令处理
- OLED 刷新
- 门锁行为控制

### `MQTT`

联网扩展层，负责：

- Wi-Fi 初始化
- MQTT 建连
- 状态发布
- 指令订阅

## 当前任务列表

系统启动后会创建以下任务：

- `task_buzzer`：蜂鸣器控制
- `task_servo`：舵机/门锁控制
- `task_uart`：串口命令处理
- `task_dht11`：温湿度采集
- `task_rc522`：刷卡识别
- `task_mq2`：气体检测
- `task_sw180`：振动检测
- `task_oled`：OLED 刷新

## 运行时状态中心

项目实现了统一状态中心 `task_state`，用于保存以下数据：

- DHT11 温湿度状态
- MQ-2 状态
- 振动状态
- 最近刷到的卡 UID
- 门锁状态

这样 UART、OLED、MQTT 都可以读取同一份运行态数据，避免重复采样和状态不一致。

## 默认引脚配置

默认引脚定义位于 `savebox_board.h`：

- 蜂鸣器：`GPIO36`
- DHT11：`GPIO2`
- 舵机：`GPIO35`
- SW180：`GPIO37`
- MQ-2 ADC：`GPIO1`
- RC522 MOSI：`GPIO42`
- RC522 MISO：`GPIO40`
- RC522 SCK：`GPIO14`
- RC522 NSS：`GPIO39`
- RC522 RST：`GPIO38`
- OLED SCL：`GPIO21`
- OLED SDA：`GPIO47`
- UART：`UART0`

## 当前推荐默认配置

针对当前已经验证过的 `ESP32-S3-CAM + 自定义传感器扩展板` 组合，推荐默认配置为：

- 保持 `camera`、`Wi-Fi/MQTT`、`DHT11`、`MQ2`、`SW180` 可用
- 保持 `SAVEBOX_ENABLE_BSP_RC522_SPI_BUS = 0`
- 保持 `SAVEBOX_ENABLE_TASK_RC522 = 0`
- 保持 `SAVEBOX_ENABLE_BSP_BUZZER_GPIO_INIT = 0`
- 保持 `SAVEBOX_ENABLE_BSP_SW180_GPIO_INIT = 0`
- 保持 `SAVEBOX_ENABLE_TASK_DHT11 = 1`
- 保持 `SAVEBOX_ENABLE_TASK_MQ2 = 1`
- 保持 `SAVEBOX_ENABLE_TASK_SW180 = 1`

这样配置的原因是：

- 当前硬件上，`RC522 SPI` 默认引脚分配仍然存在冲突风险，打开后会干扰系统稳定启动
- `GPIO36`、`GPIO37` 在 `BSP_Init()` 阶段过早初始化时，会影响相机初始化稳定性
- 但在任务阶段再使用 `SW180`，以及在当前配置下运行 `DHT11/MQ2`，已经实测可以正常工作

换句话说，当前仓库最稳的默认目标不是“所有模块全开”，而是“先保证 camera + network + 基础传感器链路稳定”，`RC522` 后续再单独重分配引脚

白名单卡 UID 默认值也定义在 `savebox_board.h` 中：

- `64-6E-1C-06`

## 目录结构

```text
savebox/
├─ ESP_APP/      设备功能模块
├─ ESP_BSP/      板级支持与兼容层
├─ MQTT/         Wi-Fi + MQTT 模块
├─ compat/       兼容头文件
├─ main/         程序入口与组件注册
├─ task/         FreeRTOS 业务任务
├─ savebox_board.h
├─ sdkconfig
└─ README.md
```

## 启动流程

`app_main()` 当前启动流程如下：

1. 打印芯片信息
2. 初始化 BSP
3. 初始化 APP 功能模块
4. 初始化运行时状态中心
5. 启动全部任务
6. 启动 MQTT 模块

## 使用说明

### 本地运行

1. 连接硬件模块
2. 根据实际接线修改 `savebox_board.h`
3. 编译并烧录工程
4. 打开串口监视器查看日志和输入命令

### MQTT 使用

1. 打开 `MQTT/savebox_mqtt_config.h`
2. 填写 Wi-Fi 名称和密码
3. 填写 Broker 地址、客户端 ID、设备 ID
4. 根据需要修改 topic
5. 重新编译烧录

## 代码现状说明

当前项目已经具备完整的本地闭环：

- 传感器采集
- 门锁控制
- 本地串口交互
- OLED 可视化
- 报警联动

同时已经具备 MQTT 扩展基础，可以继续往以下方向扩展：

- 手机 App 或小程序联动
- 云端设备管理
- 远程开锁记录
- 历史数据存储
- TLS 安全连接
- 多卡白名单管理

## 主要源码入口

- `main/main.c`：程序入口
- `ESP_BSP/bsp_platform.c`：平台适配核心
- `task/task_state.c`：运行时状态中心
- `task/task_uart.c`：UART 命令处理
- `task/task_rc522.c`：刷卡控制逻辑
- `task/task_mq2.c`：气体检测逻辑
- `task/task_sw180.c`：振动检测逻辑
- `task/task_oled.c`：OLED 页面刷新
- `MQTT/savebox_mqtt.c`：MQTT 联网逻辑
