#ifndef __APP_MQ2_H
#define __APP_MQ2_H

#include <stdbool.h>

#include "bsp_adc.h"
#include "esp_err.h"

typedef struct {
    int raw;
    int voltage_mv;
    bool alarm;
} mq2_reading_t;

esp_err_t MQ2_Init(void);
esp_err_t MQ2_Read(mq2_reading_t *reading);
int MQ2_GetAlarmThresholdRaw(void);

#endif
