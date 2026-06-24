#include "task_buzzer.h"

#include "driver/gpio.h"
#include "bsp_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "savebox_board.h"

#define TASK_BUZZER_STACK_SIZE 2048
#define TASK_BUZZER_PRIORITY 3
#define TASK_BUZZER_QUEUE_LENGTH 8

static QueueHandle_t s_buzzer_queue = NULL;

static void task_buzzer_entry(void *arg)
{
    uint32_t duration_ms = 0;

    (void)arg;

    bsp_gpio_writePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);

    for (;;) {
        if (xQueueReceive(s_buzzer_queue, &duration_ms, portMAX_DELAY) == pdTRUE) {
            bsp_gpio_writePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(duration_ms));
            bsp_gpio_writePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
        }
    }
}

void task_buzzer_start(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << Buzzer_Pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (s_buzzer_queue != NULL) {
        return;
    }

    gpio_config(&config);
    gpio_set_level((gpio_num_t)Buzzer_Pin, 1);

    s_buzzer_queue = xQueueCreate(TASK_BUZZER_QUEUE_LENGTH, sizeof(uint32_t));
    if (s_buzzer_queue == NULL) {
        return;
    }

    xTaskCreate(task_buzzer_entry, "task_buzzer", TASK_BUZZER_STACK_SIZE, NULL, TASK_BUZZER_PRIORITY, NULL);
}

bool task_buzzer_beep(uint32_t duration_ms)
{
    if (s_buzzer_queue == NULL) {
        return false;
    }

    return xQueueSend(s_buzzer_queue, &duration_ms, 0) == pdTRUE;
}
