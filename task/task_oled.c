#include "task_oled.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "app_oled.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_state.h"

#define TASK_OLED_STACK_SIZE 4096
#define TASK_OLED_PRIORITY 2
#define TASK_OLED_PERIOD_MS 1000
#define TASK_OLED_LINE_COUNT 8U
#define TASK_OLED_LINE_CHAR_CAPACITY 16U

static bool s_oled_task_started = false;

static bool task_oled_prepare_line(char cache[TASK_OLED_LINE_COUNT][TASK_OLED_LINE_CHAR_CAPACITY + 1U],
                                   uint8_t line,
                                   const char *text,
                                   char padded[TASK_OLED_LINE_CHAR_CAPACITY + 1U])
{
    size_t src_len = 0U;

    memset(padded, ' ', TASK_OLED_LINE_CHAR_CAPACITY);
    padded[TASK_OLED_LINE_CHAR_CAPACITY] = '\0';

    if (text != NULL) {
        src_len = strlen(text);
        if (src_len > TASK_OLED_LINE_CHAR_CAPACITY) {
            src_len = TASK_OLED_LINE_CHAR_CAPACITY;
        }
        memcpy(padded, text, src_len);
    }

    if (strncmp(cache[line], padded, TASK_OLED_LINE_CHAR_CAPACITY) == 0) {
        return false;
    }

    memcpy(cache[line], padded, TASK_OLED_LINE_CHAR_CAPACITY + 1U);
    return true;
}

static void task_oled_update_line(char cache[TASK_OLED_LINE_COUNT][TASK_OLED_LINE_CHAR_CAPACITY + 1U],
                                  uint8_t line,
                                  const char *text,
                                  char padded[TASK_OLED_LINE_CHAR_CAPACITY + 1U])
{
    if (task_oled_prepare_line(cache, line, text, padded)) {
        OLED_ShowString8x8(0, line, padded);
    }
}

static void task_oled_entry(void *arg)
{
    savebox_lock_state_t lock_state = {0};
    savebox_dht11_state_t dht11_state = {0};
    savebox_mq2_state_t mq2_state = {0};
    savebox_vibration_state_t vibration_state = {0};
    savebox_card_state_t card_state = {0};
    char line_cache[TASK_OLED_LINE_COUNT][TASK_OLED_LINE_CHAR_CAPACITY + 1U] = {{0}};
    char line_buffer[TASK_OLED_LINE_CHAR_CAPACITY + 1U] = {0};
    char padded[TASK_OLED_LINE_CHAR_CAPACITY + 1U] = {0};

    (void)arg;

    OLED_Init();
    OLED_Clear();

    for (;;) {
        task_state_get_lock(&lock_state);
        task_state_get_dht11(&dht11_state);
        task_state_get_mq2(&mq2_state);
        task_state_get_vibration(&vibration_state);
        task_state_get_card(&card_state);

        task_oled_update_line(line_cache,
                              0U,
                              lock_state.locked ? "LOCK=LOCKED" : "LOCK=UNLOCKED",
                              padded);

        if (dht11_state.valid) {
            snprintf(line_buffer, sizeof(line_buffer), "TEMP=%u.%uC", dht11_state.temp_int, dht11_state.temp_dec);
            task_oled_update_line(line_cache, 1U, line_buffer, padded);
            snprintf(line_buffer, sizeof(line_buffer), "HUMI=%u.%u", dht11_state.humi_int, dht11_state.humi_dec);
            task_oled_update_line(line_cache, 2U, line_buffer, padded);
        } else {
            task_oled_update_line(line_cache, 1U, "TEMP=NA", padded);
            task_oled_update_line(line_cache, 2U, "HUMI=NA", padded);
        }

        if (mq2_state.valid) {
            snprintf(line_buffer, sizeof(line_buffer), "MQ2=%d", mq2_state.raw);
            task_oled_update_line(line_cache, 3U, line_buffer, padded);
            snprintf(line_buffer, sizeof(line_buffer), "DELTA=%d", mq2_state.delta_raw);
            task_oled_update_line(line_cache, 4U, line_buffer, padded);
            if (mq2_state.warmup_active) {
                snprintf(line_buffer,
                         sizeof(line_buffer),
                         "WARM=%luS",
                         (unsigned long)((mq2_state.warmup_remaining_ms + 999U) / 1000U));
                task_oled_update_line(line_cache, 5U, line_buffer, padded);
            } else {
                task_oled_update_line(line_cache, 5U, mq2_state.alarm ? "GAS=ALARM" : "GAS=NORMAL", padded);
            }
        } else {
            task_oled_update_line(line_cache, 3U, "MQ2=NA", padded);
            task_oled_update_line(line_cache, 4U, "DELTA=NA", padded);
            task_oled_update_line(line_cache, 5U, "GAS=NA", padded);
        }

        if (vibration_state.valid) {
            task_oled_update_line(line_cache, 6U, vibration_state.alarm ? "VIB=ALARM" : "VIB=NORMAL", padded);
        } else {
            task_oled_update_line(line_cache, 6U, "VIB=NA", padded);
        }

        if (card_state.valid) {
            snprintf(line_buffer,
                     sizeof(line_buffer),
                     "CARD=%02X%02X%02X%02X",
                     card_state.uid[0],
                     card_state.uid[1],
                     card_state.uid[2],
                     card_state.uid[3]);
            task_oled_update_line(line_cache, 7U, line_buffer, padded);
        } else {
            task_oled_update_line(line_cache, 7U, "CARD=NONE", padded);
        }
        vTaskDelay(pdMS_TO_TICKS(TASK_OLED_PERIOD_MS));
    }
}

void task_oled_start(void)
{
    if (s_oled_task_started) {
        return;
    }

    s_oled_task_started = true;
    xTaskCreate(task_oled_entry, "task_oled", TASK_OLED_STACK_SIZE, NULL, TASK_OLED_PRIORITY, NULL);
}
