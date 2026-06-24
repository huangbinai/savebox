#include "savebox_http_upload.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "savebox_mqtt_config.h"
#include "task_state.h"

static const char *TAG = "savebox_http";

static bool savebox_http_url_configured(const char *url)
{
    return (url != NULL) &&
           (url[0] != '\0') &&
           (strncmp(url, "http://192.168.31.120:3000", strlen("http://192.168.31.120:3000")) != 0);
}

static esp_err_t savebox_http_post_binary(const char *url,
                                          const uint8_t *data,
                                          size_t len,
                                          const char *frame_type,
                                          const char *alarm_type,
                                          uint32_t captured_at_ms)
{
    char captured_at[16] = {0};
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = SAVEBOX_HTTP_REQUEST_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = NULL;
    esp_err_t err = ESP_FAIL;

    if ((url == NULL) || (data == NULL) || (len == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_FAIL;
    }

    snprintf(captured_at, sizeof(captured_at), "%" PRIu32, captured_at_ms);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "type", frame_type);
    esp_http_client_set_header(client, "X-Savebox-Device-Id", SAVEBOX_MQTT_DEVICE_ID);
    esp_http_client_set_header(client, "X-Savebox-Frame-Type", frame_type);
    esp_http_client_set_header(client, "X-Savebox-Captured-At-Ms", captured_at);
    if (alarm_type != NULL) {
        esp_http_client_set_header(client, "X-Savebox-Alarm-Type", alarm_type);
    }
    esp_http_client_set_post_field(client, (const char *)data, (int)len);

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        const int status = esp_http_client_get_status_code(client);
        if (status < 200 || status >= 300) {
            err = ESP_FAIL;
        }
    }

    esp_http_client_cleanup(client);
    return err;
}

static esp_err_t savebox_http_post_json(const char *url, const char *json)
{
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = SAVEBOX_HTTP_REQUEST_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = NULL;
    esp_err_t err = ESP_FAIL;

    if ((url == NULL) || (json == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    client = esp_http_client_init(&config);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json, (int)strlen(json));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        const int status = esp_http_client_get_status_code(client);
        if (status < 200 || status >= 300) {
            err = ESP_FAIL;
        }
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t savebox_http_upload_latest_frame(const uint8_t *data, size_t len, uint32_t captured_at_ms)
{
    if (!savebox_http_url_configured(SAVEBOX_HTTP_LATEST_FRAME_URL)) {
        return ESP_ERR_INVALID_STATE;
    }

    return savebox_http_post_binary(SAVEBOX_HTTP_LATEST_FRAME_URL,
                                    data,
                                    len,
                                    "normal",
                                    NULL,
                                    captured_at_ms);
}

esp_err_t savebox_http_upload_alarm_frame(const uint8_t *data,
                                          size_t len,
                                          const char *alarm_type,
                                          uint32_t captured_at_ms)
{
    if (!savebox_http_url_configured(SAVEBOX_HTTP_ALARM_FRAME_URL)) {
        return ESP_ERR_INVALID_STATE;
    }

    return savebox_http_post_binary(SAVEBOX_HTTP_ALARM_FRAME_URL,
                                    data,
                                    len,
                                    "alarm",
                                    alarm_type,
                                    captured_at_ms);
}

esp_err_t savebox_http_upload_status_snapshot(void)
{
    savebox_dht11_state_t dht11 = {0};
    savebox_mq2_state_t mq2 = {0};
    savebox_vibration_state_t vibration = {0};
    savebox_card_state_t card = {0};
    savebox_lock_state_t lock = {0};
    savebox_camera_state_t camera = {0};
    cJSON *root = NULL;
    char *json = NULL;
    esp_err_t err = ESP_FAIL;

    if (!savebox_http_url_configured(SAVEBOX_HTTP_STATUS_URL)) {
        return ESP_ERR_INVALID_STATE;
    }

    task_state_get_dht11(&dht11);
    task_state_get_mq2(&mq2);
    task_state_get_vibration(&vibration);
    task_state_get_card(&card);
    task_state_get_lock(&lock);
    task_state_get_camera(&camera);

    root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "deviceId", SAVEBOX_MQTT_DEVICE_ID);
    cJSON_AddNumberToObject(root, "uptimeMs", (double)(esp_timer_get_time() / 1000ULL));
    cJSON_AddBoolToObject(root, "lockState", lock.locked);
    cJSON_AddBoolToObject(root, "cardDetected", card.valid);
    cJSON_AddBoolToObject(root, "cameraReady", camera.initialized);
    cJSON_AddBoolToObject(root, "cameraLastCaptureOk", camera.last_capture_ok);
    cJSON_AddBoolToObject(root, "cameraLastUploadOk", camera.last_upload_ok);
    cJSON_AddNumberToObject(root, "cameraLastCaptureMs", camera.last_capture_ms);
    cJSON_AddNumberToObject(root, "cameraLastUploadMs", camera.last_upload_ms);

    if (dht11.valid) {
        cJSON_AddNumberToObject(root, "temperature", (double)dht11.temp_int + ((double)dht11.temp_dec / 10.0));
        cJSON_AddNumberToObject(root, "humidity", (double)dht11.humi_int + ((double)dht11.humi_dec / 10.0));
    }

    if (mq2.valid) {
        cJSON_AddNumberToObject(root, "gasRaw", mq2.raw);
        cJSON_AddNumberToObject(root, "gasVoltageMv", mq2.voltage_mv);
        cJSON_AddNumberToObject(root, "gasDeltaRaw", mq2.delta_raw);
        cJSON_AddBoolToObject(root, "gasAlarm", mq2.alarm);
        cJSON_AddBoolToObject(root, "gasWarmupActive", mq2.warmup_active);
    }

    if (vibration.valid) {
        cJSON_AddBoolToObject(root, "vibrationAlarm", vibration.alarm);
        cJSON_AddNumberToObject(root, "vibrationLevel", vibration.level);
    }

    json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    err = savebox_http_post_json(SAVEBOX_HTTP_STATUS_URL, json);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "status upload failed: %s", esp_err_to_name(err));
    }

    cJSON_free(json);
    cJSON_Delete(root);
    return err;
}
