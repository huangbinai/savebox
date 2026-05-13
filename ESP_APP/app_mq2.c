#include "app_mq2.h"

#include "savebox_board.h"

#define MQ2_SAMPLE_COUNT 8

static BSP_ADC_HandleTypeDef s_mq2_adc = {
    .unit_handle = NULL,
    .unit_id = SAVEBOX_MQ2_ADC_UNIT,
    .channel = SAVEBOX_MQ2_ADC_CHANNEL,
    .atten = SAVEBOX_MQ2_ADC_ATTEN,
    .bitwidth = ADC_BITWIDTH_12,
    .cali_handle = NULL,
    .initialized = false,
    .cali_enabled = false,
};

esp_err_t MQ2_Init(void)
{
    return bsp_adc_init(&s_mq2_adc);
}

esp_err_t MQ2_Read(mq2_reading_t *reading)
{
    esp_err_t err = ESP_OK;
    int raw_sum = 0;
    int mv_sum = 0;
    int i = 0;

    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_mq2_adc.initialized) {
        err = MQ2_Init();
        if (err != ESP_OK) {
            return err;
        }
    }

    for (i = 0; i < MQ2_SAMPLE_COUNT; ++i) {
        int raw_value = 0;
        int voltage_mv = 0;

        err = bsp_adc_read_raw(&s_mq2_adc, &raw_value);
        if (err != ESP_OK) {
            return err;
        }

        err = bsp_adc_read_mv(&s_mq2_adc, &voltage_mv);
        if (err != ESP_OK) {
            return err;
        }

        raw_sum += raw_value;
        mv_sum += voltage_mv;
    }

    reading->raw = raw_sum / MQ2_SAMPLE_COUNT;
    reading->voltage_mv = mv_sum / MQ2_SAMPLE_COUNT;
    reading->alarm = false;

    return ESP_OK;
}

int MQ2_GetAlarmThresholdRaw(void)
{
    return SAVEBOX_MQ2_DELTA_ALARM_THRESHOLD_RAW;
}