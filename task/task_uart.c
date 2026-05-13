#include "task_uart.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "app_rc522.h"
#include "bsp_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_servo.h"
#include "task_state.h"

#define TASK_UART_STACK_SIZE 4096
#define TASK_UART_PRIORITY 5
#define TASK_UART_RX_TIMEOUT_MS 100U
#define TASK_UART_CMD_BUF_SIZE 64U

static bool s_uart_task_started = false;

static bool task_uart_is_single_char_command(char command_char)
{
    return (command_char == 'L') || (command_char == 'U') || (command_char == 'S') ||
           (command_char == 'H') || (command_char == '?');
}

static size_t task_uart_trim_copy(char *dst, size_t dst_size, const char *src, size_t src_len)
{
    size_t begin = 0U;
    size_t end = src_len;
    size_t out_len = 0U;

    while ((begin < src_len) && isspace((unsigned char)src[begin])) {
        begin++;
    }
    while ((end > begin) && isspace((unsigned char)src[end - 1U])) {
        end--;
    }

    out_len = end - begin;
    if (out_len >= dst_size) {
        out_len = dst_size - 1U;
    }

    memcpy(dst, src + begin, out_len);
    dst[out_len] = '\0';
    return out_len;
}

static void task_uart_send_help(void)
{
    UART_Printf(&huart1,
                "Commands: L/U, LOCK/UNLOCK, STATUS, DHT?, MQ2?, CARD?, RC522?, HELP\r\n");
}

static void task_uart_send_status(void)
{
    savebox_lock_state_t lock_state = {0};
    savebox_dht11_state_t dht11_state = {0};
    savebox_mq2_state_t mq2_state = {0};
    savebox_card_state_t card_state = {0};

    task_state_get_lock(&lock_state);
    task_state_get_dht11(&dht11_state);
    task_state_get_mq2(&mq2_state);
    task_state_get_card(&card_state);

    UART_Printf(&huart1, "LOCK=%s\r\n", lock_state.locked ? "LOCKED" : "UNLOCKED");

    if (dht11_state.valid) {
        UART_Printf(&huart1,
                    "DHT11=%u.%uC,%u.%u%%RH\r\n",
                    dht11_state.temp_int,
                    dht11_state.temp_dec,
                    dht11_state.humi_int,
                    dht11_state.humi_dec);
    } else {
        UART_Printf(&huart1, "DHT11=NA\r\n");
    }

    if (mq2_state.valid) {
        UART_Printf(&huart1,
                    "MQ2=raw:%d,mv:%d,delta:%d,warmup:%s(%lu),alarm:%s\r\n",
                    mq2_state.raw,
                    mq2_state.voltage_mv,
                    mq2_state.delta_raw,
                    mq2_state.warmup_active ? "ON" : "OFF",
                    (unsigned long)mq2_state.warmup_remaining_ms,
                    mq2_state.alarm ? "YES" : "NO");
    } else {
        UART_Printf(&huart1, "MQ2=NA\r\n");
    }

    if (card_state.valid) {
        UART_Printf(&huart1,
                    "CARD=%02X-%02X-%02X-%02X\r\n",
                    card_state.uid[0],
                    card_state.uid[1],
                    card_state.uid[2],
                    card_state.uid[3]);
    } else {
        UART_Printf(&huart1, "CARD=NA\r\n");
    }
}

static void task_uart_handle_command(const char *command)
{
    if ((strcmp(command, "L") == 0) || (strcmp(command, "LOCK") == 0)) {
        UART_Printf(&huart1, task_servo_lock() ? "OK LOCK\r\n" : "ERR LOCK\r\n");
        return;
    }

    if ((strcmp(command, "U") == 0) || (strcmp(command, "UNLOCK") == 0)) {
        UART_Printf(&huart1, task_servo_unlock() ? "OK UNLOCK\r\n" : "ERR UNLOCK\r\n");
        return;
    }

    if ((strcmp(command, "S") == 0) || (strcmp(command, "STATUS") == 0)) {
        task_uart_send_status();
        return;
    }

    if ((strcmp(command, "DHT?") == 0) || (strcmp(command, "DHT11?") == 0)) {
        savebox_dht11_state_t dht11_state = {0};
        task_state_get_dht11(&dht11_state);
        if (dht11_state.valid) {
            UART_Printf(&huart1,
                        "DHT11=%u.%uC,%u.%u%%RH\r\n",
                        dht11_state.temp_int,
                        dht11_state.temp_dec,
                        dht11_state.humi_int,
                        dht11_state.humi_dec);
        } else {
            UART_Printf(&huart1, "DHT11=NA\r\n");
        }
        return;
    }

    if (strcmp(command, "MQ2?") == 0) {
        savebox_mq2_state_t mq2_state = {0};
        task_state_get_mq2(&mq2_state);
        if (mq2_state.valid) {
            UART_Printf(&huart1,
                        "MQ2=raw:%d,mv:%d,delta:%d,warmup:%s(%lu),alarm:%s\r\n",
                        mq2_state.raw,
                        mq2_state.voltage_mv,
                        mq2_state.delta_raw,
                        mq2_state.warmup_active ? "ON" : "OFF",
                        (unsigned long)mq2_state.warmup_remaining_ms,
                        mq2_state.alarm ? "YES" : "NO");
        } else {
            UART_Printf(&huart1, "MQ2=NA\r\n");
        }
        return;
    }

    if (strcmp(command, "CARD?") == 0) {
        savebox_card_state_t card_state = {0};
        task_state_get_card(&card_state);
        if (card_state.valid) {
            UART_Printf(&huart1,
                        "CARD=%02X-%02X-%02X-%02X\r\n",
                        card_state.uid[0],
                        card_state.uid[1],
                        card_state.uid[2],
                        card_state.uid[3]);
        } else {
            UART_Printf(&huart1, "CARD=NA\r\n");
        }
        return;
    }

    if ((strcmp(command, "RC522?") == 0) || (strcmp(command, "VERSION?") == 0)) {
        UART_Printf(&huart1, "RC522_VERSION=0x%02X\r\n", RC522_ReadRawRC(VersionReg));
        return;
    }

    if ((strcmp(command, "H") == 0) || (strcmp(command, "HELP") == 0) || (strcmp(command, "?") == 0)) {
        task_uart_send_help();
        return;
    }

    UART_Printf(&huart1, "ERR UNKNOWN CMD: %s\r\n", command);
}

static void task_uart_process_buffer(const char *buffer, size_t length)
{
    char command[TASK_UART_CMD_BUF_SIZE];

    if (task_uart_trim_copy(command, sizeof(command), buffer, length) == 0U) {
        return;
    }

    task_uart_handle_command(command);
}

static void task_uart_entry(void *arg)
{
    uint8_t rx_byte = 0U;
    char command_buffer[TASK_UART_CMD_BUF_SIZE];
    size_t command_length = 0U;

    (void)arg;

    task_uart_send_help();

    for (;;) {
        const int received = UART_ReadBytes(&huart1, &rx_byte, 1U, TASK_UART_RX_TIMEOUT_MS);

        if (received <= 0) {
            if ((command_length == 1U) && task_uart_is_single_char_command(command_buffer[0])) {
                task_uart_process_buffer(command_buffer, command_length);
                command_length = 0U;
            }
            continue;
        }

        if ((rx_byte == '\r') || (rx_byte == '\n')) {
            if (command_length > 0U) {
                task_uart_process_buffer(command_buffer, command_length);
                command_length = 0U;
            }
            continue;
        }

        if (command_length < (sizeof(command_buffer) - 1U)) {
            command_buffer[command_length++] = (char)rx_byte;
            command_buffer[command_length] = '\0';
        } else {
            UART_Printf(&huart1, "ERR CMD TOO LONG\r\n");
            command_length = 0U;
        }
    }
}

void task_uart_start(void)
{
    if (s_uart_task_started) {
        return;
    }

    if (!huart1.use_driver || !huart1.initialized) {
        printf("[uart_trace] task_uart disabled because no dedicated UART driver is active\n");
        return;
    }

    s_uart_task_started = true;
    xTaskCreate(task_uart_entry, "task_uart", TASK_UART_STACK_SIZE, NULL, TASK_UART_PRIORITY, NULL);
}
