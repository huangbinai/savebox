#ifndef __APP_SW180_H
#define __APP_SW180_H

#include <stdbool.h>

#include "compat/gpio.h"
#include "esp_err.h"
#include "bsp_uart.h"
esp_err_t SW180_Init(void);
GPIO_PinState SW180_ReadDO(void);
bool SW180_IsTriggered(void);

#endif