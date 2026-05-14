#include "task_sw180.h"

#include <stdbool.h>
#include <stdio.h>

#include "app_sw180.h"
#include "bsp_platform.h"
#include "bsp_uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "savebox_board.h"
#include "task_buzzer.h"
#include "task_camera.h"
#include "task_state.h"

#define TASK_SW180_STACK_SIZE 2048
#define TASK_SW180_PRIORITY 4
#define TASK_SW180_STATUS_CHECK_MS 50
#define TASK_SW180_ALARM_BEEP_MS 2000
#define TASK_SW180_DEBOUNCE_MS 50

static bool s_sw180_task_started = false;

static void task_sw180_entry(void *arg)
{
    bool previous_triggered = false;
    uint32_t last_alarm_ms = 0U;
    uint32_t last_state_change_ms = 0U;

    (void)arg;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(TASK_SW180_STATUS_CHECK_MS));

        const GPIO_PinState level = SW180_ReadDO();
        const bool triggered = (level == SAVEBOX_SW180_TRIGGER_LEVEL);
        const uint32_t now_ms = HAL_GetTick();

        task_state_set_vibration((int)level, triggered);

        if (!previous_triggered && triggered && ((now_ms - last_state_change_ms) >= TASK_SW180_DEBOUNCE_MS)) {
            UART_Printf(&huart1, "SW180 DO=%d\r\n", (int)level);
            UART_Printf(&huart1, "Vibration!\r\n");

            if ((now_ms - last_alarm_ms) >= TASK_SW180_ALARM_BEEP_MS) {
                task_buzzer_beep(TASK_SW180_ALARM_BEEP_MS);
                task_camera_notify_alarm(SAVEBOX_CAMERA_ALARM_VIBRATION);
                last_alarm_ms = now_ms;
            }

            last_state_change_ms = now_ms;
        }

        if (previous_triggered && !triggered) {
            UART_Printf(&huart1, "SW180 DO=%d\r\n", (int)level);
            UART_Printf(&huart1, "Status Normal\r\n");
            last_state_change_ms = now_ms;
        }

        previous_triggered = triggered;
    }
}

void task_sw180_start(void)
{
    if (s_sw180_task_started) {
        return;
    }

    s_sw180_task_started = true;
    xTaskCreate(task_sw180_entry, "task_sw180", TASK_SW180_STACK_SIZE, NULL, TASK_SW180_PRIORITY, NULL);
}

