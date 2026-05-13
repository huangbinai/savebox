#include "bsp_uart.h"

void UART_SendChar(UART_HandleTypeDef *huart, char ch)
{
    HAL_UART_Transmit(huart, (const uint8_t *)&ch, 1, HAL_MAX_DELAY);
}

void UART_SendString(UART_HandleTypeDef *huart, const char *msg)
{
    HAL_UART_Transmit(huart, (const uint8_t *)msg, (uint16_t)strlen(msg), HAL_MAX_DELAY);
}

void UART_Printf(UART_HandleTypeDef *huart, const char *format, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(huart, (const uint8_t *)buffer, (uint16_t)strlen(buffer), HAL_MAX_DELAY);
}

int UART_ReadBytes(UART_HandleTypeDef *huart, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    return savebox_uart_read(huart, data, len, timeout_ms);
}

int LOG_Debug_Out(UART_HandleTypeDef *huart, const char *__file, const char *__func, int __line, const char *format, ...)
{
    char log_prefix[128] = {0};
    char log_message[512] = {0};
    va_list args;

    snprintf(log_prefix, sizeof(log_prefix), "[%s Func:%s Line:%d] ", __file, __func, __line);
    strncpy(log_message, log_prefix, sizeof(log_message) - 1);

    va_start(args, format);
    vsnprintf(log_message + strlen(log_message), sizeof(log_message) - strlen(log_message), format, args);
    va_end(args);

    strncat(log_message, "\r\n", sizeof(log_message) - strlen(log_message) - 1);
    HAL_UART_Transmit(huart, (const uint8_t *)log_message, (uint16_t)strlen(log_message), HAL_MAX_DELAY);

    return (int)strlen(log_message);
}