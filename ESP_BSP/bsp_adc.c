#include "bsp_adc.h"

#include <stdint.h>

#include "esp_adc/adc_cali_scheme.h"

static esp_err_t bsp_adc_create_cali(BSP_ADC_HandleTypeDef *hadc)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = hadc->unit_id,
        .atten = hadc->atten,
        .bitwidth = hadc->bitwidth,
    };

    return adc_cali_create_scheme_curve_fitting(&cali_config, &hadc->cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = hadc->unit_id,
        .atten = hadc->atten,
        .bitwidth = hadc->bitwidth,
    };

    return adc_cali_create_scheme_line_fitting(&cali_config, &hadc->cali_handle);
#else
    (void)hadc;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t bsp_adc_init(BSP_ADC_HandleTypeDef *hadc)
{
    esp_err_t err = ESP_OK;

    if (hadc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (hadc->initialized) {
        return ESP_OK;
    }

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = hadc->unit_id,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    err = adc_oneshot_new_unit(&unit_config, &hadc->unit_handle);
    if (err != ESP_OK) {
        return err;
    }

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = hadc->atten,
        .bitwidth = hadc->bitwidth,
    };

    err = adc_oneshot_config_channel(hadc->unit_handle, hadc->channel, &channel_config);
    if (err != ESP_OK) {
        return err;
    }

    err = bsp_adc_create_cali(hadc);
    hadc->cali_enabled = (err == ESP_OK);
    hadc->initialized = true;

    return ESP_OK;
}

esp_err_t bsp_adc_read_raw(BSP_ADC_HandleTypeDef *hadc, int *raw_value)
{
    if ((hadc == NULL) || (raw_value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!hadc->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return adc_oneshot_read(hadc->unit_handle, hadc->channel, raw_value);
}

esp_err_t bsp_adc_read_mv(BSP_ADC_HandleTypeDef *hadc, int *voltage_mv)
{
    int raw_value = 0;
    esp_err_t err = ESP_OK;

    if ((hadc == NULL) || (voltage_mv == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    err = bsp_adc_read_raw(hadc, &raw_value);
    if (err != ESP_OK) {
        return err;
    }

    if (hadc->cali_enabled) {
        return adc_cali_raw_to_voltage(hadc->cali_handle, raw_value, voltage_mv);
    }

    *voltage_mv = (int)(((int64_t)raw_value * 3300LL) / 4095LL);
    return ESP_OK;
}
