#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include <stdbool.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

typedef struct {
    adc_oneshot_unit_handle_t unit_handle;
    adc_unit_t unit_id;
    adc_channel_t channel;
    adc_atten_t atten;
    adc_bitwidth_t bitwidth;
    adc_cali_handle_t cali_handle;
    bool initialized;
    bool cali_enabled;
} BSP_ADC_HandleTypeDef;

esp_err_t bsp_adc_init(BSP_ADC_HandleTypeDef *hadc);
esp_err_t bsp_adc_read_raw(BSP_ADC_HandleTypeDef *hadc, int *raw_value);
esp_err_t bsp_adc_read_mv(BSP_ADC_HandleTypeDef *hadc, int *voltage_mv);

#endif
