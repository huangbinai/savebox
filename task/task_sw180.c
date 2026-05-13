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
static TaskHandle_t s_sw180_task_handle = NULL;

static void IRAM_ATTR task_sw180_isr_handler(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    (void)arg;

    if (s_sw180_task_handle != NULL) {
        vTaskNotifyGiveFromISR(s_sw180_task_handle, &higher_priority_task_woken);
    }

    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void task_sw180_entry(void *arg)
{
    bool previous_triggered = false;
    uint32_t last_alarm_ms = 0U;
    uint32_t last_irq_ms = 0U;
    esp_err_t err = ESP_OK;

    (void)arg;

    err = gpio_install_isr_service(0);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        printf("[task_sw180] isr service install failed: %s\n", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    gpio_set_intr_type((gpio_num_t)SW180_DO_Pin, GPIO_INTR_NEGEDGE);
    gpio_intr_disable((gpio_num_t)SW180_DO_Pin);
    gpio_isr_handler_remove((gpio_num_t)SW180_DO_Pin);
    err = gpio_isr_handler_add((gpio_num_t)SW180_DO_Pin, task_sw180_isr_handler, NULL);
    if (err != ESP_OK) {
        printf("[task_sw180] isr add failed: %s\n", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    gpio_intr_enable((gpio_num_t)SW180_DO_Pin);

    for (;;) {
        const bool irq_fired = (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(TASK_SW180_STATUS_CHECK_MS)) > 0U);
        const GPIO_PinState level = SW180_ReadDO();
        const bool triggered = (level == SAVEBOX_SW180_TRIGGER_LEVEL);
        const uint32_t now_ms = HAL_GetTick();

        task_state_set_vibration((int)level, triggered);

        if (irq_fired && triggered && ((now_ms - last_irq_ms) >= TASK_SW180_DEBOUNCE_MS)) {
            UART_Printf(&huart1, "SW180 DO=%d\r\n", (int)level);
            UART_Printf(&huart1, "Vibration!\r\n");

            if ((now_ms - last_alarm_ms) >= TASK_SW180_ALARM_BEEP_MS) {
                task_buzzer_beep(TASK_SW180_ALARM_BEEP_MS);
                task_camera_notify_alarm(SAVEBOX_CAMERA_ALARM_VIBRATION);
                last_alarm_ms = now_ms;
            }

            last_irq_ms = now_ms;
        }

        if (previous_triggered && !triggered) {
            UART_Printf(&huart1, "SW180 DO=%d\r\n", (int)level);
            UART_Printf(&huart1, "Status Normal\r\n");
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
    xTaskCreate(task_sw180_entry, "task_sw180", TASK_SW180_STACK_SIZE, NULL, TASK_SW180_PRIORITY, &s_sw180_task_handle);
}

