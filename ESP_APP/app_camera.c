#include "app_camera.h"

#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "savebox_board.h"

static const char *TAG = "app_camera";
static bool s_camera_ready = false;
static SemaphoreHandle_t s_camera_mutex = NULL;

static camera_config_t savebox_camera_build_config(void)
{
    camera_config_t config = {0};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = SAVEBOX_CAMERA_D0_GPIO;
    config.pin_d1 = SAVEBOX_CAMERA_D1_GPIO;
    config.pin_d2 = SAVEBOX_CAMERA_D2_GPIO;
    config.pin_d3 = SAVEBOX_CAMERA_D3_GPIO;
    config.pin_d4 = SAVEBOX_CAMERA_D4_GPIO;
    config.pin_d5 = SAVEBOX_CAMERA_D5_GPIO;
    config.pin_d6 = SAVEBOX_CAMERA_D6_GPIO;
    config.pin_d7 = SAVEBOX_CAMERA_D7_GPIO;
    config.pin_xclk = SAVEBOX_CAMERA_XCLK_GPIO;
    config.pin_pclk = SAVEBOX_CAMERA_PCLK_GPIO;
    config.pin_vsync = SAVEBOX_CAMERA_VSYNC_GPIO;
    config.pin_href = SAVEBOX_CAMERA_HREF_GPIO;
    config.pin_sccb_sda = SAVEBOX_CAMERA_SIOD_GPIO;
    config.pin_sccb_scl = SAVEBOX_CAMERA_SIOC_GPIO;
    config.pin_pwdn = SAVEBOX_CAMERA_PWDN_GPIO;
    config.pin_reset = SAVEBOX_CAMERA_RESET_GPIO;
    config.xclk_freq_hz = SAVEBOX_CAMERA_XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
    return config;
}

static void savebox_camera_apply_sensor_tuning(void)
{
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL) {
        return;
    }

    sensor->set_vflip(sensor, 1);
    sensor->set_hmirror(sensor, 0);
    sensor->set_brightness(sensor, 0);
    sensor->set_saturation(sensor, 0);
}

static esp_err_t savebox_camera_lock(void)
{
    if (s_camera_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return xSemaphoreTake(s_camera_mutex, portMAX_DELAY) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void savebox_camera_unlock(void)
{
    if (s_camera_mutex != NULL) {
        xSemaphoreGive(s_camera_mutex);
    }
}

esp_err_t savebox_camera_init(void)
{
    camera_config_t config = {0};
    esp_err_t err = ESP_OK;

#if !SAVEBOX_CAMERA_ENABLE
    return ESP_ERR_NOT_SUPPORTED;
#endif

    if (s_camera_ready) {
        return ESP_OK;
    }

    if (s_camera_mutex == NULL) {
        s_camera_mutex = xSemaphoreCreateMutex();
        if (s_camera_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    config = savebox_camera_build_config();

    printf("[camera_trace] config reset=%d pwdn=%d xclk=%d siod=%d sioc=%d\n",
           (int)config.pin_reset,
           (int)config.pin_pwdn,
           (int)config.pin_xclk,
           (int)config.pin_sccb_sda,
           (int)config.pin_sccb_scl);
    printf("[camera_trace] config d0=%d d1=%d d2=%d d3=%d d4=%d d5=%d d6=%d d7=%d\n",
           (int)config.pin_d0,
           (int)config.pin_d1,
           (int)config.pin_d2,
           (int)config.pin_d3,
           (int)config.pin_d4,
           (int)config.pin_d5,
           (int)config.pin_d6,
           (int)config.pin_d7);
    printf("[camera_trace] config vsync=%d href=%d pclk=%d xclk_hz=%d\n",
           (int)config.pin_vsync,
           (int)config.pin_href,
           (int)config.pin_pclk,
           (int)config.xclk_freq_hz);
    printf("[camera_trace] config fb_location=%d fb_count=%d frame_size=%d psram=%d\n",
           (int)config.fb_location,
           config.fb_count,
           config.frame_size,
           0);

    err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: %s", esp_err_to_name(err));
        return err;
    }

    savebox_camera_apply_sensor_tuning();
    s_camera_ready = true;
    ESP_LOGI(TAG, "camera ready");
    return ESP_OK;
}

bool savebox_camera_is_ready(void)
{
    return s_camera_ready;
}

camera_fb_t *savebox_camera_capture_jpeg(void)
{
    esp_err_t err = ESP_OK;

    if (!s_camera_ready) {
        return NULL;
    }

    err = savebox_camera_lock();
    if (err != ESP_OK) {
        return NULL;
    }

    return esp_camera_fb_get();
}

void savebox_camera_return_frame(camera_fb_t *fb)
{
    if (fb != NULL) {
        esp_camera_fb_return(fb);
    }

    savebox_camera_unlock();
}
