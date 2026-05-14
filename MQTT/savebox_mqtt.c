#include "savebox_mqtt.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "bsp_uart.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "savebox_mqtt_config.h"
#include "task_buzzer.h"
#include "task_servo.h"
#include "task_state.h"

#define SAVEBOX_WIFI_CONNECTED_BIT BIT0
#define SAVEBOX_MQTT_CONNECTED_BIT BIT1

static const char *TAG = "savebox_mqtt";

static EventGroupHandle_t s_event_group = NULL;
static esp_event_handler_instance_t s_wifi_event_instance = NULL;
static esp_event_handler_instance_t s_ip_event_instance = NULL;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static esp_netif_t *s_sta_netif = NULL;
static TaskHandle_t s_publish_task_handle = NULL;
static bool s_wifi_ready = false;
static bool s_started = false;
static uint32_t s_wifi_retry_count = 0;

static bool savebox_mqtt_configured(void)
{
    return (SAVEBOX_WIFI_SSID[0] != '\0') &&
           (strcmp(SAVEBOX_WIFI_SSID, "YOUR_WIFI_SSID") != 0) &&
           (SAVEBOX_MQTT_BROKER_URI[0] != '\0') &&
           (SAVEBOX_MQTT_CLIENT_ID[0] != '\0') &&
           (SAVEBOX_MQTT_DEVICE_ID[0] != '\0') &&
           (SAVEBOX_MQTT_SERVICE_ID[0] != '\0');
}

bool savebox_mqtt_is_connected(void)
{
    if (s_event_group == NULL) {
        return false;
    }

    return (xEventGroupGetBits(s_event_group) & SAVEBOX_MQTT_CONNECTED_BIT) != 0;
}

static esp_err_t savebox_mqtt_publish_raw(const char *topic, const char *payload)
{
    if ((topic == NULL) || (payload == NULL) || (s_mqtt_client == NULL) || !savebox_mqtt_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }

    return (esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, 1, 0) >= 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t savebox_mqtt_publish_json(const char *topic, cJSON *root)
{
    char *payload = NULL;
    esp_err_t err = ESP_FAIL;

    if ((topic == NULL) || (root == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    payload = cJSON_PrintUnformatted(root);
    if (payload == NULL) {
        return ESP_ERR_NO_MEM;
    }

    err = savebox_mqtt_publish_raw(topic, payload);
    cJSON_free(payload);
    return err;
}

static esp_err_t savebox_mqtt_publish_device_message(const char *event_text)
{
    cJSON *root = NULL;
    esp_err_t err = ESP_FAIL;

    if (event_text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "deviceId", SAVEBOX_MQTT_DEVICE_ID);
    cJSON_AddStringToObject(root, "event", event_text);
    cJSON_AddNumberToObject(root, "uptimeMs", (double)(esp_timer_get_time() / 1000ULL));

    err = savebox_mqtt_publish_json(SAVEBOX_MQTT_MESSAGES_UP_TOPIC, root);
    cJSON_Delete(root);
    return err;
}

static bool savebox_mqtt_extract_request_id(const char *topic, char *request_id, size_t request_id_size)
{
    const char *marker = NULL;

    if ((topic == NULL) || (request_id == NULL) || (request_id_size == 0U)) {
        return false;
    }

    marker = strstr(topic, SAVEBOX_MQTT_COMMAND_REQUEST_ID_MARKER);
    if (marker == NULL) {
        return false;
    }

    marker += strlen(SAVEBOX_MQTT_COMMAND_REQUEST_ID_MARKER);
    if (*marker == '\0') {
        return false;
    }

    snprintf(request_id, request_id_size, "%s", marker);
    return true;
}

static esp_err_t savebox_mqtt_publish_command_response(const char *request_id, int result_code, const char *result_text)
{
    char topic[192] = {0};
    cJSON *root = NULL;
    cJSON *paras = NULL;
    esp_err_t err = ESP_FAIL;

    if ((request_id == NULL) || (result_text == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(topic, sizeof(topic), "%s%s", SAVEBOX_MQTT_COMMAND_RESPONSE_TOPIC_PREFIX, request_id);

    root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddNumberToObject(root, "result_code", result_code);
    cJSON_AddStringToObject(root, "response_name", SAVEBOX_MQTT_COMMAND_RESPONSE_NAME);
    paras = cJSON_AddObjectToObject(root, "paras");
    if (paras == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(paras, "result", result_text);
    err = savebox_mqtt_publish_json(topic, root);
    cJSON_Delete(root);
    return err;
}

static esp_err_t savebox_mqtt_execute_command(const char *command_name, cJSON *paras)
{
    esp_err_t err = ESP_OK;
    bool ok = false;

    if (command_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(command_name, SAVEBOX_MQTT_COMMAND_LOCK) == 0) {
        cJSON *lock_item = NULL;
        bool target_lock = true;

        if (paras != NULL) {
            lock_item = cJSON_GetObjectItemCaseSensitive(paras, SAVEBOX_MQTT_COMMAND_LOCK_PARAM);
        }

        if ((lock_item != NULL) && cJSON_IsBool(lock_item)) {
            target_lock = cJSON_IsTrue(lock_item);
        }

        ok = target_lock ? task_servo_lock() : task_servo_unlock();
        UART_Printf(&huart1,
                    ok ? (target_lock ? "MQTT LOCK OK\r\n" : "MQTT UNLOCK OK\r\n")
                       : (target_lock ? "MQTT LOCK FAIL\r\n" : "MQTT UNLOCK FAIL\r\n"));
        return ok ? ESP_OK : ESP_FAIL;
    }

    if (strcmp(command_name, SAVEBOX_MQTT_COMMAND_SYNC_STATUS) == 0) {
        UART_Printf(&huart1, "MQTT SYNC STATUS\r\n");
        return savebox_mqtt_publish_snapshot();
    }

    if (strcmp(command_name, SAVEBOX_MQTT_COMMAND_BEEP) == 0) {
        uint32_t duration_ms = SAVEBOX_MQTT_DEFAULT_BEEP_MS;
        cJSON *duration_item = NULL;

        if (paras != NULL) {
            duration_item = cJSON_GetObjectItemCaseSensitive(paras, SAVEBOX_MQTT_COMMAND_BEEP_PARAM);
            if (cJSON_IsNumber(duration_item) && (duration_item->valuedouble > 0.0)) {
                duration_ms = (uint32_t)duration_item->valuedouble;
            }
        }

        ok = task_buzzer_beep(duration_ms);
        UART_Printf(&huart1, ok ? "MQTT BEEP OK\r\n" : "MQTT BEEP FAIL\r\n");
        err = ok ? ESP_OK : ESP_FAIL;
        if (err == ESP_OK) {
            err = savebox_mqtt_publish_snapshot();
        }
        return err;
    }

    return ESP_ERR_NOT_SUPPORTED;
}

static void savebox_mqtt_handle_command(const char *topic, const char *data)
{
    cJSON *root = NULL;
    cJSON *command_name_item = NULL;
    cJSON *service_id_item = NULL;
    cJSON *paras_item = NULL;
    char request_id[128] = {0};
    const char *command_name = NULL;
    const char *service_id = NULL;
    esp_err_t exec_err = ESP_FAIL;

    if ((topic == NULL) || (data == NULL)) {
        return;
    }

    if (!savebox_mqtt_extract_request_id(topic, request_id, sizeof(request_id))) {
        ESP_LOGW(TAG, "Ignore non-command topic: %s", topic);
        return;
    }

    root = cJSON_Parse(data);
    if (root == NULL) {
        ESP_LOGW(TAG, "Invalid command payload: %s", data);
        savebox_mqtt_publish_command_response(request_id, 1, "invalid_json");
        return;
    }

    command_name_item = cJSON_GetObjectItemCaseSensitive(root, "command_name");
    service_id_item = cJSON_GetObjectItemCaseSensitive(root, "service_id");
    paras_item = cJSON_GetObjectItemCaseSensitive(root, "paras");

    command_name = cJSON_GetStringValue(command_name_item);
    service_id = cJSON_GetStringValue(service_id_item);

    if (command_name == NULL) {
        savebox_mqtt_publish_command_response(request_id, 1, "missing_command_name");
        cJSON_Delete(root);
        return;
    }

    if ((service_id != NULL) && (strcmp(service_id, SAVEBOX_MQTT_SERVICE_ID) != 0)) {
        savebox_mqtt_publish_command_response(request_id, 1, "service_id_mismatch");
        cJSON_Delete(root);
        return;
    }

    exec_err = savebox_mqtt_execute_command(command_name, cJSON_IsObject(paras_item) ? paras_item : NULL);
    savebox_mqtt_publish_command_response(request_id, (exec_err == ESP_OK) ? 0 : 1, (exec_err == ESP_OK) ? "success" : "failed");

    if (exec_err == ESP_OK) {
        savebox_mqtt_publish_device_message(command_name);
    }

    cJSON_Delete(root);
}

static void savebox_mqtt_event_handler(void *handler_args,
                                       esp_event_base_t base,
                                       int32_t event_id,
                                       void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    (void)handler_args;
    (void)base;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(s_event_group, SAVEBOX_MQTT_CONNECTED_BIT);
            esp_mqtt_client_subscribe(s_mqtt_client, SAVEBOX_MQTT_COMMAND_SUB_TOPIC, 1);
            UART_Printf(&huart1, "IoTDA MQTT connected\r\n");
            savebox_mqtt_publish_device_message("mqtt_connected");
            savebox_mqtt_publish_snapshot();
            break;

        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(s_event_group, SAVEBOX_MQTT_CONNECTED_BIT);
            UART_Printf(&huart1, "IoTDA MQTT disconnected\r\n");
            break;

        case MQTT_EVENT_DATA:
            if ((event != NULL) && (event->topic != NULL) && (event->data != NULL)) {
                char topic[256] = {0};
                char data[512] = {0};
                const int topic_len = (event->topic_len < (int)(sizeof(topic) - 1)) ? event->topic_len : (int)(sizeof(topic) - 1);
                const int data_len = (event->data_len < (int)(sizeof(data) - 1)) ? event->data_len : (int)(sizeof(data) - 1);

                memcpy(topic, event->topic, (size_t)topic_len);
                memcpy(data, event->data, (size_t)data_len);
                UART_Printf(&huart1, "IoTDA CMD RX: %s\r\n", topic);
                savebox_mqtt_handle_command(topic, data);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "MQTT event error");
            break;

        default:
            break;
    }
}

static esp_err_t savebox_mqtt_start_client(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = SAVEBOX_MQTT_BROKER_URI,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .credentials.username = (SAVEBOX_MQTT_USERNAME[0] != '\0') ? SAVEBOX_MQTT_USERNAME : NULL,
        .credentials.authentication.password = (SAVEBOX_MQTT_PASSWORD[0] != '\0') ? SAVEBOX_MQTT_PASSWORD : NULL,
        .credentials.client_id = SAVEBOX_MQTT_CLIENT_ID,
        .session.keepalive = SAVEBOX_MQTT_KEEPALIVE_SECONDS,
        .network.reconnect_timeout_ms = 5000,
    };

    if (s_mqtt_client != NULL) {
        return ESP_OK;
    }

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client,
                                                   ESP_EVENT_ANY_ID,
                                                   savebox_mqtt_event_handler,
                                                   NULL));

    return esp_mqtt_client_start(s_mqtt_client);
}

static void savebox_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START)) {
        esp_wifi_connect();
        return;
    }

    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
        xEventGroupClearBits(s_event_group, SAVEBOX_WIFI_CONNECTED_BIT | SAVEBOX_MQTT_CONNECTED_BIT);

        if (s_wifi_retry_count < SAVEBOX_MQTT_WIFI_MAXIMUM_RETRY) {
            s_wifi_retry_count++;
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "Wi-Fi reconnect retries reached %lu", (unsigned long)s_wifi_retry_count);
        }
        return;
    }

    if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
        s_wifi_retry_count = 0;
        xEventGroupSetBits(s_event_group, SAVEBOX_WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Wi-Fi connected, starting MQTT client");
        if (s_mqtt_client == NULL) {
            esp_err_t err = savebox_mqtt_start_client();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "savebox_mqtt_start_client failed: %s", esp_err_to_name(err));
            }
        }
    }
}

static esp_err_t savebox_wifi_init(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {0};
    esp_err_t err = ESP_OK;

    if (s_wifi_ready) {
        printf("[mqtt_trace] wifi already ready\n");
        return ESP_OK;
    }

    printf("[mqtt_trace] before esp_netif_init\n");
    err = esp_netif_init();
    printf("[mqtt_trace] after esp_netif_init err=%s\n", esp_err_to_name(err));
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return err;
    }

    printf("[mqtt_trace] before esp_event_loop_create_default\n");
    err = esp_event_loop_create_default();
    printf("[mqtt_trace] after esp_event_loop_create_default err=%s\n", esp_err_to_name(err));
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return err;
    }

    if (s_sta_netif == NULL) {
        printf("[mqtt_trace] before esp_netif_create_default_wifi_sta\n");
        s_sta_netif = esp_netif_create_default_wifi_sta();
        if (s_sta_netif == NULL) {
            printf("[mqtt_trace] esp_netif_create_default_wifi_sta failed\n");
            return ESP_FAIL;
        }
        printf("[mqtt_trace] after esp_netif_create_default_wifi_sta\n");
    }

    printf("[mqtt_trace] before esp_wifi_init\n");
    err = esp_wifi_init(&cfg);
    printf("[mqtt_trace] after esp_wifi_init err=%s\n", esp_err_to_name(err));
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return err;
    }

    printf("[mqtt_trace] before register WIFI_EVENT handler\n");
    err = esp_event_handler_instance_register(WIFI_EVENT,
                                              ESP_EVENT_ANY_ID,
                                              &savebox_wifi_event_handler,
                                              NULL,
                                              &s_wifi_event_instance);
    printf("[mqtt_trace] after register WIFI_EVENT handler err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    printf("[mqtt_trace] before register IP_EVENT handler\n");
    err = esp_event_handler_instance_register(IP_EVENT,
                                              IP_EVENT_STA_GOT_IP,
                                              &savebox_wifi_event_handler,
                                              NULL,
                                              &s_ip_event_instance);
    printf("[mqtt_trace] after register IP_EVENT handler err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", SAVEBOX_WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", SAVEBOX_WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode = (SAVEBOX_WIFI_PASSWORD[0] != '\0') ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    printf("[mqtt_trace] before esp_wifi_set_mode\n");
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    printf("[mqtt_trace] after esp_wifi_set_mode err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    printf("[mqtt_trace] before esp_wifi_set_storage\n");
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    printf("[mqtt_trace] after esp_wifi_set_storage err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    printf("[mqtt_trace] before esp_wifi_set_config\n");
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    printf("[mqtt_trace] after esp_wifi_set_config err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    printf("[mqtt_trace] before esp_wifi_start\n");
    err = esp_wifi_start();
    printf("[mqtt_trace] after esp_wifi_start err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    s_wifi_ready = true;
    return ESP_OK;
}

static void savebox_mqtt_publish_task(void *arg)
{
    (void)arg;

    for (;;) {
        xEventGroupWaitBits(s_event_group,
                            SAVEBOX_MQTT_CONNECTED_BIT,
                            pdFALSE,
                            pdFALSE,
                            portMAX_DELAY);
        savebox_mqtt_publish_snapshot();
        vTaskDelay(pdMS_TO_TICKS(SAVEBOX_MQTT_PUBLISH_PERIOD_MS));
    }
}

esp_err_t savebox_mqtt_publish_snapshot(void)
{
    savebox_dht11_state_t dht11 = {0};
    savebox_mq2_state_t mq2 = {0};
    savebox_vibration_state_t vibration = {0};
    savebox_card_state_t card = {0};
    savebox_lock_state_t lock = {0};
    char uid[16] = {0};
    cJSON *root = NULL;
    cJSON *services = NULL;
    cJSON *service = NULL;
    cJSON *properties = NULL;
    esp_err_t err = ESP_FAIL;

    if ((s_mqtt_client == NULL) || !savebox_mqtt_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }

    task_state_get_dht11(&dht11);
    task_state_get_mq2(&mq2);
    task_state_get_vibration(&vibration);
    task_state_get_card(&card);
    task_state_get_lock(&lock);

    if (card.valid) {
        snprintf(uid,
                 sizeof(uid),
                 "%02X%02X%02X%02X",
                 card.uid[0],
                 card.uid[1],
                 card.uid[2],
                 card.uid[3]);
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    services = cJSON_AddArrayToObject(root, "services");
    if (services == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    service = cJSON_CreateObject();
    if (service == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddItemToArray(services, service);

    cJSON_AddStringToObject(service, "service_id", SAVEBOX_MQTT_SERVICE_ID);
    properties = cJSON_AddObjectToObject(service, "properties");
    if (properties == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddBoolToObject(properties, "lockState", lock.locked);
    if (dht11.valid) {
        cJSON_AddNumberToObject(properties, "temperature", (double)dht11.temp_int + ((double)dht11.temp_dec / 10.0));
        cJSON_AddNumberToObject(properties, "humidity", (double)dht11.humi_int + ((double)dht11.humi_dec / 10.0));
    }

    if (mq2.valid) {
        cJSON_AddNumberToObject(properties, "gasRaw", mq2.raw);
        cJSON_AddNumberToObject(properties, "gasVoltageMv", mq2.voltage_mv);
        cJSON_AddNumberToObject(properties, "gasDeltaRaw", mq2.delta_raw);
        cJSON_AddBoolToObject(properties, "gasAlarm", mq2.alarm);
        cJSON_AddBoolToObject(properties, "gasWarmupActive", mq2.warmup_active);
        cJSON_AddNumberToObject(properties, "gasWarmupRemainingMs", mq2.warmup_remaining_ms);
    }

    if (vibration.valid) {
        cJSON_AddBoolToObject(properties, "vibrationAlarm", vibration.alarm);
        cJSON_AddNumberToObject(properties, "vibrationLevel", vibration.level);
    }

    cJSON_AddBoolToObject(properties, "cardDetected", card.valid);
    if (card.valid) {
        cJSON_AddStringToObject(properties, "lastCardUid", uid);
    }

    cJSON_AddNumberToObject(properties, "uptimeMs", (double)(esp_timer_get_time() / 1000ULL));

    err = savebox_mqtt_publish_json(SAVEBOX_MQTT_PROPERTIES_REPORT_TOPIC, root);
    cJSON_Delete(root);
    return err;
}

esp_err_t savebox_mqtt_start(void)
{
    esp_err_t err = ESP_OK;

    if (s_started) {
        printf("[mqtt_trace] mqtt already started\n");
        return ESP_OK;
    }

    if (!savebox_mqtt_configured()) {
        ESP_LOGW(TAG, "MQTT disabled: fill Wi-Fi and IoTDA settings in MQTT/savebox_mqtt_config.h");
        return ESP_OK;
    }

    if (s_event_group == NULL) {
        printf("[mqtt_trace] before xEventGroupCreate\n");
        s_event_group = xEventGroupCreate();
        if (s_event_group == NULL) {
            return ESP_ERR_NO_MEM;
        }
        printf("[mqtt_trace] after xEventGroupCreate\n");
    }

    printf("[mqtt_trace] before nvs_flash_init\n");
    err = nvs_flash_init();
    printf("[mqtt_trace] after nvs_flash_init err=%s\n", esp_err_to_name(err));
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err != ESP_OK) {
            return erase_err;
        }
        err = nvs_flash_init();
        printf("[mqtt_trace] after nvs_flash_erase+nvs_flash_init err=%s\n", esp_err_to_name(err));
    }
    if (err != ESP_OK) {
        return err;
    }

    printf("[mqtt_trace] before savebox_wifi_init\n");
    err = savebox_wifi_init();
    printf("[mqtt_trace] after savebox_wifi_init err=%s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
        return err;
    }

    if (s_publish_task_handle == NULL) {
        printf("[mqtt_trace] before create mqtt_publish task\n");
        if (xTaskCreate(savebox_mqtt_publish_task,
                        "mqtt_publish",
                        4096,
                        NULL,
                        4,
                        &s_publish_task_handle) != pdPASS) {
            return ESP_ERR_NO_MEM;
        }
        printf("[mqtt_trace] after create mqtt_publish task\n");
    }

    s_started = true;
    ESP_LOGI(TAG, "IoTDA MQTT module started");
    return ESP_OK;
}
