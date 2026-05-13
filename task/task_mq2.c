#include "task_mq2.h"

#include <stdbool.h>
#include <stdio.h>

#include "app_mq2.h"
#include "bsp_platform.h"
#include "bsp_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "savebox_board.h"
#include "task_buzzer.h"
#include "task_camera.h"
#include "task_state.h"

#define TASK_MQ2_STACK_SIZE 3072
#define TASK_MQ2_PRIORITY 4
#define TASK_MQ2_PERIOD_MS 1000
#define TASK_MQ2_ALARM_BEEP_MS 2000
#define TASK_MQ2_HISTORY_SIZE 4

typedef struct {
    bool valid;
    int raw;
    uint32_t tick_ms;
} mq2_history_sample_t;

static void task_mq2_push_history(mq2_history_sample_t history[TASK_MQ2_HISTORY_SIZE],
                                  size_t *count,
                                  size_t *next_index,
                                  int raw,
                                  uint32_t tick_ms)
{
    history[*next_index].valid = true;
    history[*next_index].raw = raw;
    history[*next_index].tick_ms = tick_ms;

    *next_index = (*next_index + 1U) % TASK_MQ2_HISTORY_SIZE;
    if (*count < TASK_MQ2_HISTORY_SIZE) {
        (*count)++;
    }
}

static bool task_mq2_get_reference_raw(const mq2_history_sample_t history[TASK_MQ2_HISTORY_SIZE],
                                       size_t count,
                                       uint32_t now_ms,
                                       int *reference_raw)
{
    bool found = false;
    uint32_t newest_tick_ms = 0U;
    size_t i = 0U;

    for (i = 0U; i < count; ++i) {
        if (!history[i].valid) {
            continue;
        }

        if ((now_ms - history[i].tick_ms) < SAVEBOX_MQ2_DELTA_WINDOW_MS) {
            continue;
        }

        if (!found || (history[i].tick_ms > newest_tick_ms)) {
            *reference_raw = history[i].raw;
            newest_tick_ms = history[i].tick_ms;
            found = true;
        }
    }

    return found;
}

static void task_mq2_entry(void *arg)
{
    mq2_reading_t reading = {0};
    mq2_history_sample_t history[TASK_MQ2_HISTORY_SIZE] = {0};
    size_t history_count = 0U;
    size_t history_next_index = 0U;
    uint32_t start_ms = HAL_GetTick();
    bool previous_alarm = false;

    (void)arg;

    for (;;) {
        if (MQ2_Read(&reading) == ESP_OK) {
            const uint32_t now_ms = HAL_GetTick();
            const uint32_t elapsed_ms = now_ms - start_ms;
            const bool warmup_active = (elapsed_ms < SAVEBOX_MQ2_WARMUP_MS);
            const uint32_t warmup_remaining_ms = warmup_active ? (SAVEBOX_MQ2_WARMUP_MS - elapsed_ms) : 0U;
            int reference_raw = reading.raw;
            int delta_raw = 0;
            bool alarm = false;

            if (!warmup_active &&
                task_mq2_get_reference_raw(history, history_count, now_ms, &reference_raw)) {
                delta_raw = reading.raw - reference_raw;
                alarm = (delta_raw >= SAVEBOX_MQ2_DELTA_ALARM_THRESHOLD_RAW);
            }

            task_state_set_mq2(reading.raw,
                               reading.voltage_mv,
                               delta_raw,
                               warmup_active,
                               warmup_remaining_ms,
                               alarm,
                               now_ms);

            printf("[task_mq2] raw=%d voltage=%dmV delta=%d warmup=%lu alarm=%s\n",
                   reading.raw,
                   reading.voltage_mv,
                   delta_raw,
                   (unsigned long)warmup_remaining_ms,
                   alarm ? "YES" : "NO");
            UART_Printf(&huart1,
                        "MQ2,%d,%d,%d,%lu,%d\r\n",
                        reading.raw,
                        reading.voltage_mv,
                        delta_raw,
                        (unsigned long)warmup_remaining_ms,
                        alarm ? 1 : 0);

            if (alarm && !previous_alarm) {
                task_buzzer_beep(TASK_MQ2_ALARM_BEEP_MS);
                task_camera_notify_alarm(SAVEBOX_CAMERA_ALARM_GAS);
            }
            previous_alarm = alarm;

            task_mq2_push_history(history,
                                  &history_count,
                                  &history_next_index,
                                  reading.raw,
                                  now_ms);
        } else {
            previous_alarm = false;
            printf("[task_mq2] read failed\n");
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_MQ2_PERIOD_MS));
    }
}

void task_mq2_start(void)
{
    xTaskCreate(task_mq2_entry, "task_mq2", TASK_MQ2_STACK_SIZE, NULL, TASK_MQ2_PRIORITY, NULL);
}
