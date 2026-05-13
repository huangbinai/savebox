#include "task_dht11.h"

#include <stdio.h>

#include "app_DHT11.h"
#include "bsp_platform.h"
#include "bsp_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_state.h"

#define TASK_DHT11_STACK_SIZE 3072
#define TASK_DHT11_PRIORITY 4
#define TASK_DHT11_PERIOD_MS 3000

static void task_dht11_entry(void *arg)
{
    uint8_t temp_int = 0;
    uint8_t temp_dec = 0;
    uint8_t humi_int = 0;
    uint8_t humi_dec = 0;

    (void)arg;

    for (;;) {
        const uint8_t status = DHT_Read(&temp_int, &temp_dec, &humi_int, &humi_dec);
        if (status == 0U) {
            task_state_set_dht11(temp_int, temp_dec, humi_int, humi_dec, HAL_GetTick());
            printf("[task_dht11] temp=%u.%uC humi=%u.%u%%\n", temp_int, temp_dec, humi_int, humi_dec);
            UART_Printf(&huart1, "DHT11,%u,%u,%u,%u\r\n", humi_int, humi_dec, temp_int, temp_dec);
        } else {
            printf("[task_dht11] read failed, status=%u\n", status);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_DHT11_PERIOD_MS));
    }
}

void task_dht11_start(void)
{
    xTaskCreate(task_dht11_entry, "task_dht11", TASK_DHT11_STACK_SIZE, NULL, TASK_DHT11_PRIORITY, NULL);
}