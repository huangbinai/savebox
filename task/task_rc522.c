#include "task_rc522.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_rc522.h"
#include "bsp_platform.h"
#include "bsp_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "savebox_board.h"
#include "task_servo.h"
#include "task_state.h"

#define TASK_RC522_STACK_SIZE 4096
#define TASK_RC522_PRIORITY 4
#define TASK_RC522_PERIOD_MS 300

static const uint8_t s_white_card_uid[4] = {
    SAVEBOX_RC522_WHITE_UID0,
    SAVEBOX_RC522_WHITE_UID1,
    SAVEBOX_RC522_WHITE_UID2,
    SAVEBOX_RC522_WHITE_UID3,
};

static void task_rc522_entry(void *arg)
{
    u8 tag_type[2] = {0};
    u8 uid[4] = {0};
    u8 last_uid[4] = {0};
    bool last_uid_valid = false;
    uint32_t last_card_time = 0U;

    (void)arg;

    for (;;) {
        if ((RC522_PcdRequest(PICC_REQIDL, tag_type) == MI_OK) &&
            (RC522_PcdAnticoll(uid) == MI_OK)) {
            const uint32_t now = HAL_GetTick();
            const bool same_as_last = last_uid_valid && (memcmp(uid, last_uid, sizeof(uid)) == 0);
            const bool is_white_card = (memcmp(uid, s_white_card_uid, sizeof(uid)) == 0);

            task_state_set_card(uid, now);

            if (!same_as_last) {
                memcpy(last_uid, uid, sizeof(uid));
                last_uid_valid = true;
                printf("[task_rc522] card uid=%02X-%02X-%02X-%02X\n", uid[0], uid[1], uid[2], uid[3]);
                UART_Printf(&huart1, "CARD,%02X,%02X,%02X,%02X\r\n", uid[0], uid[1], uid[2], uid[3]);
            }

            RC522_PcdSelect(uid);
            RC522_PcdHalt();

            if (is_white_card) {
                if ((!same_as_last) || ((now - last_card_time) > SAVEBOX_RC522_CARD_COOLDOWN_MS)) {
                    savebox_lock_state_t lock_state = {0};
                    last_card_time = now;
                    task_state_get_lock(&lock_state);

                    if (lock_state.locked) {
                        if (task_servo_unlock()) {
                            UART_Printf(&huart1, "Door unlocked by card\r\n");
                        }
                    } else {
                        if (task_servo_lock()) {
                            UART_Printf(&huart1, "Door locked by card\r\n");
                        }
                    }
                } else {
                    UART_Printf(&huart1, "Card ignored (cooldown)\r\n");
                }
            } else if (!same_as_last) {
                UART_Printf(&huart1, "Unknown card\r\n");
            }
        } else {
            last_uid_valid = false;
            task_state_clear_card();
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_RC522_PERIOD_MS));
    }
}

void task_rc522_start(void)
{
    xTaskCreate(task_rc522_entry, "task_rc522", TASK_RC522_STACK_SIZE, NULL, TASK_RC522_PRIORITY, NULL);
}