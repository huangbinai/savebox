# MQTT 模块说明

这个目录保存 Savebox 的 MQTT 接入代码和配置。

## 目录结构

- `savebox_mqtt_config.h`：Wi-Fi、Broker、Topic 和发布周期配置
- `savebox_mqtt.h/.c`：Wi-Fi 初始化、MQTT 连接、状态发布、指令订阅

## MQTT 的工作原理

MQTT 是典型的发布/订阅模型，不是设备之间直接互连。

- 设备端是 `Publisher` / `Subscriber`
- 中间服务器是 `Broker`
- 消息按 `Topic` 分类路由

通信链路通常是这样：

1. ESP32 先连上 Wi-Fi，拿到 IP。
2. ESP32 再通过 TCP 和 MQTT Broker 建立连接。
3. 连接建立时，设备会带上 `client_id`、心跳参数、可选用户名密码、遗嘱消息。
4. 设备把传感器状态 `publish` 到某个 topic。
5. 需要控制设备时，手机端或服务器端往控制 topic `publish` 指令。
6. ESP32 提前 `subscribe` 了控制 topic，所以收到后执行本地动作。

这个模型的关键点：

- `Broker` 负责转发，设备之间不需要彼此知道 IP
- `Topic` 是逻辑通道，比如 `savebox/savebox-esp32/state`
- `QoS` 决定消息可靠性和性能权衡
- `Retain` 让新订阅者一上来就能收到最后一条保留消息
- `Keepalive` 让 Broker 判断客户端是否掉线
- `Last Will` 让设备异常断开时，Broker 自动替它发一条离线消息

## 本实现做了什么

当前实现包含 3 类 topic：

- 状态上报：`SAVEBOX_MQTT_STATE_TOPIC`
- 事件通知：`SAVEBOX_MQTT_EVENT_TOPIC`
- 下行控制：`SAVEBOX_MQTT_COMMAND_TOPIC`

连接成功后会：

1. 发布在线状态到 `availability` topic
2. 订阅命令 topic
3. 立即发布一次状态快照
4. 后续按固定周期继续上报 `task_state` 中的运行态数据

当前支持的控制命令正文：

- `LOCK`
- `UNLOCK`
- `PING`

## 开发方法流程

建议按下面顺序做 MQTT 开发：

1. 先定义设备需要上报什么，控制什么。
2. 设计 topic 规范，不要把所有消息混在一个 topic。
3. 设计 payload 格式，优先 JSON，便于调试。
4. 先打通 Wi-Fi，再打通 Broker 连接，再做 publish/subscribe。
5. 先做状态上报，再做下行控制，最后再做重连、异常处理和鉴权。

在这个工程里，对应关系是：

1. 运行态数据源来自 `task_state`
2. 上行动作在 `savebox_mqtt_publish_snapshot()`
3. 下行入口在 `savebox_mqtt_handle_command()`
4. 实际开锁/关锁复用了 `task_servo_lock()` / `task_servo_unlock()`

## 使用方法

先修改 `savebox_mqtt_config.h`：

- 填上 `SAVEBOX_WIFI_SSID`
- 填上 `SAVEBOX_WIFI_PASSWORD`
- 按需要修改 Broker URI 和 Topic

如果你本机装了 `mosquitto`，可以这样测试：

```bash
mosquitto_sub -h broker.emqx.io -t savebox/savebox-esp32/state -v
mosquitto_sub -h broker.emqx.io -t savebox/savebox-esp32/event -v
mosquitto_pub -h broker.emqx.io -t savebox/savebox-esp32/command -m UNLOCK
mosquitto_pub -h broker.emqx.io -t savebox/savebox-esp32/command -m LOCK
```

## 后续可继续扩展

- 把配置改成 `menuconfig` 而不是写死在头文件
- 改成 TLS：`mqtts://`
- 给 topic 增加设备序列号和产品线分层
- 根据不同传感器变化事件做增量发布，而不是只做周期上报
