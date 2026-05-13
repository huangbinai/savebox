# Savebox 摄像头 HTTP 接入说明

## 本版本新增内容

- 为 `ESP32-S3-CAM` 增加摄像头采集能力。
- 增加周期上传“最新图片”到本地后端的能力。
- 增加报警时立即抓拍并上传“报警帧”的能力。
- 增加周期上传设备状态快照到本地后端的能力。
- 在 `task_state` 中增加摄像头运行状态记录。

## 本次新增文件

- `ESP_APP/app_camera.h`
- `ESP_APP/app_camera.c`
- `MQTT/savebox_http_upload.h`
- `MQTT/savebox_http_upload.c`
- `task/task_camera.h`
- `task/task_camera.c`
- `main/idf_component.yml`

## 本次修改文件

- `savebox_board.h`
- `ESP_APP/app_list.h`
- `ESP_APP/app_list.c`
- `task/task_list.c`
- `task/task_mq2.c`
- `task/task_sw180.c`
- `task/task_state.h`
- `task/task_state.c`
- `MQTT/savebox_mqtt_config.h`
- `main/CMakeLists.txt`

## 运行流程

1. `app_main()` 仍然按原来的顺序执行：
   - `BSP_Init()`
   - `APP_Init()`
   - `task_state_init()`
   - `TASK_StartAll()`
   - `savebox_mqtt_start()`
2. `APP_Init()` 现在会额外调用 `savebox_camera_init()` 初始化摄像头。
3. `TASK_StartAll()` 现在会额外启动 `task_camera`。
4. `task_camera` 负责三件事：
   - 按固定周期抓拍并上传最新图片。
   - 按固定周期上传设备状态快照。
   - 等待报警通知，收到后立即抓拍并上传报警图片。
5. `task_mq2` 和 `task_sw180` 在检测到新的报警边沿时，会通知 `task_camera` 去抓拍。

## ESP 端默认假设的 HTTP 接口

当前 ESP 代码默认本地后端提供以下接口：

- `POST /api/camera/latest-frame`
- `POST /api/camera/alarm-frame`
- `POST /api/device/status`

其中：

- 图片上传使用原始 `image/jpeg` 请求体。
- 状态上传使用 `application/json`。

图片上传时会附带这些请求头：

- `X-Savebox-Device-Id`
- `X-Savebox-Frame-Type`
- `X-Savebox-Captured-At-Ms`
- `X-Savebox-Alarm-Type`
  仅报警图片上传时附带。

## 运行前必须修改的配置

请先修改 `MQTT/savebox_mqtt_config.h` 中的这些宏：

- `SAVEBOX_HTTP_BASE_URL`
- `SAVEBOX_HTTP_LATEST_FRAME_URL`
- `SAVEBOX_HTTP_ALARM_FRAME_URL`
- `SAVEBOX_HTTP_STATUS_URL`

当前文件里的地址仍然是占位用的本地地址，不改的话 ESP 端会认为 HTTP 上传未配置完成。

## 当前使用的摄像头引脚

摄像头引脚宏定义已经加入 `savebox_board.h`，对应当前确认过的接线方案：

- `SIOD -> GPIO4`
- `SIOC -> GPIO5`
- `VSYNC -> GPIO6`
- `HREF -> GPIO7`
- `Y4 -> GPIO8`
- `Y3 -> GPIO9`
- `Y5 -> GPIO10`
- `Y2 -> GPIO11`
- `Y6 -> GPIO12`
- `PCLK -> GPIO13`
- `XCLK -> GPIO15`
- `Y9 -> GPIO16`
- `Y8 -> GPIO17`
- `Y7 -> GPIO18`
- `PWDN -> NC`
- `RESET -> NC`

## 建议如何阅读这版代码

如果你想最快看懂这一版新增功能，建议按下面顺序看：

1. `main/main.c`
   - 看整个系统启动顺序。
2. `ESP_APP/app_list.c`
   - 看系统初始化了哪些应用层模块。
3. `task/task_list.c`
   - 看系统启动了哪些任务。
4. `task/task_camera.c`
   - 看摄像头任务如何调度“周期上传”和“报警抓拍”。
5. `MQTT/savebox_http_upload.c`
   - 看图片和状态是如何通过 HTTP 发到本地后端的。
6. `ESP_APP/app_camera.c`
   - 看摄像头底层是如何初始化和抓取 JPEG 的。
7. `task/task_mq2.c` 与 `task/task_sw180.c`
   - 看报警源是怎样触发摄像头抓拍的。
8. `task/task_state.h` 与 `task/task_state.c`
   - 看系统共享运行状态是怎样定义和保存的。
9. `savebox_board.h`
   - 看这一版统一的 GPIO 定义。

## 这一版的实现思路

这次实现遵循的是“最小增量改造”：

- 不打散你现有的 `ESP_APP + task + MQTT` 结构。
- Wi-Fi 仍然复用现有的 `savebox_mqtt_start()` 初始化流程。
- 摄像头单独封装在 `app_camera`。
- 上传逻辑单独封装在 `savebox_http_upload`。
- 调度逻辑集中放在 `task_camera`。
- 报警源任务只负责“通知”，不直接处理图片上传。

这样后面你继续改：

- 上传接口格式
- 本地后端 URL
- 图片分辨率
- 上传周期

都会比较方便。

## 当前仍需注意的事项

- 这一版代码已经接入工程，但当前还没有完成本机 ESP-IDF 环境下的实际编译验证。
- 项目已经通过 `main/idf_component.yml` 声明依赖 `espressif/esp32-camera`。
- 如果后续抓图不稳定，优先检查：
  - 本机 ESP-IDF 环境是否正常
  - `esp32-camera` 组件是否成功下载
  - 板子是否启用并正确识别 `PSRAM`
  - 本地后端地址是否能被开发板访问
