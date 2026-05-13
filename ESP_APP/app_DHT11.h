#ifndef __APP_DHT11_H
#define __APP_DHT11_H

#include "compat/gpio.h"
#include "compat/usart.h"
#include "bsp_delay.h"
#include "bsp_gpio.h"
#include "bsp_uart.h"

#define DHT11_TIMEOUT 1000U

uint8_t DHT_Read(uint8_t *temp_int, uint8_t *temp_dec, uint8_t *humi_int, uint8_t *humi_dec);

#endif
