#include "bsp_delay.h"

#include "esp_rom_sys.h"          // esp_rom_delay_us
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"        // vTaskDelay, xTaskGetSchedulerState

void delay_us(uint32_t us)
{
    if (us == 0U) {
        return;
    }

    esp_rom_delay_us(us);
}

void delay_ms(uint32_t ms)
{
    if (ms == 0U) {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        TickType_t ticks = pdMS_TO_TICKS(ms);
        if (ticks == 0U) {
            ticks = 1U;
        }
        vTaskDelay(ticks);
        return;
    }

    // 调度器未启动：回退为忙等
    while (ms-- > 0U) {
        esp_rom_delay_us(1000U);
    }
}

