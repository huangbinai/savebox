#include "task_camera.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "app_camera.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "savebox_http_upload.h"
#include "savebox_mqtt.h"
#include "savebox_mqtt_config.h"
#include "task_state.h"

#define TASK_CAMERA_STACK_SIZE 6144
#define TASK_CAMERA_PRIORITY 4
#define TASK_CAMERA_NOTIFY_TIMEOUT_MS 200

static TaskHandle_t s_task_camera_handle = NULL;
static bool s_task_camera_started = false;

static const char *task_camera_alarm_type_to_text(uint32_t alarm_bits)
{
    if ((alarm_bits & SAVEBOX_CAMERA_ALARM_GAS) != 0U) {
        return "gas";
    }

    if ((alarm_bits & SAVEBOX_CAMERA_ALARM_VIBRATION) != 0U) {
        return "vibration";
    }

    return "unknown";
}

static esp_err_t task_camera_capture_and_upload_latest(uint32_t now_ms)
{
    camera_fb_t *fb = savebox_camera_capture_jpeg();
    esp_err_t err = ESP_FAIL;

    if (fb == NULL) {
        task_state_set_camera(savebox_camera_is_ready(), false, false, false, now_ms, 0U, 0U);
        return ESP_FAIL;
    }

    err = savebox_http_upload_latest_frame(fb->buf, fb->len, now_ms);
    task_state_set_camera(savebox_camera_is_ready(),
                          true,
                          (err == ESP_OK),
                          false,
                          now_ms,
                          (err == ESP_OK) ? now_ms : 0U,
                          0U);
    savebox_camera_return_frame(fb);
    return err;
}

static esp_err_t task_camera_capture_and_upload_alarm(uint32_t alarm_bits, uint32_t now_ms)
{
    camera_fb_t *fb = savebox_camera_capture_jpeg();
    esp_err_t err = ESP_FAIL;

    if (fb == NULL) {
        task_state_set_camera(savebox_camera_is_ready(), false, false, false, now_ms, 0U, 0U);
        return ESP_FAIL;
    }

    err = savebox_http_upload_alarm_frame(fb->buf,
                                          fb->len,
                                          task_camera_alarm_type_to_text(alarm_bits),
                                          now_ms);
    task_state_set_camera(savebox_camera_is_ready(),
                          true,
                          false,
                          (err == ESP_OK),
                          now_ms,
                          0U,
                          (err == ESP_OK) ? now_ms : 0U);
    savebox_camera_return_frame(fb);
    return err;
}

static void task_camera_entry(void *arg)
{
    uint32_t last_latest_upload_ms = 0U;
    uint32_t last_status_upload_ms = 0U;

    (void)arg;

    for (;;) {
        uint32_t alarm_bits = 0U;
        const uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        const bool network_ready = savebox_mqtt_is_connected();

        (void)xTaskNotifyWait(0U,
                              0xFFFFFFFFU,
                              &alarm_bits,
                              pdMS_TO_TICKS(TASK_CAMERA_NOTIFY_TIMEOUT_MS));

        if (network_ready &&
            savebox_camera_is_ready() &&
            ((now_ms - last_latest_upload_ms) >= SAVEBOX_HTTP_LATEST_FRAME_PERIOD_MS)) {
            if (task_camera_capture_and_upload_latest(now_ms) == ESP_OK) {
                last_latest_upload_ms = now_ms;
            }
        }

        if (network_ready && alarm_bits != 0U && savebox_camera_is_ready()) {
            (void)task_camera_capture_and_upload_alarm(alarm_bits, now_ms);
        }

        if (network_ready &&
            ((now_ms - last_status_upload_ms) >= SAVEBOX_HTTP_STATUS_PERIOD_MS)) {
            (void)savebox_http_upload_status_snapshot();
            last_status_upload_ms = now_ms;
        }
    }
}

void task_camera_start(void)
{
    if (s_task_camera_started) {
        return;
    }

    s_task_camera_started = true;
    xTaskCreate(task_camera_entry,
                "task_camera",
                TASK_CAMERA_STACK_SIZE,
                NULL,
                TASK_CAMERA_PRIORITY,
                &s_task_camera_handle);
}

bool task_camera_notify_alarm(savebox_camera_alarm_t alarm_type)
{
    if ((s_task_camera_handle == NULL) || (alarm_type == SAVEBOX_CAMERA_ALARM_NONE)) {
        return false;
    }

    return xTaskNotify(s_task_camera_handle, (uint32_t)alarm_type, eSetBits) == pdPASS;
}
