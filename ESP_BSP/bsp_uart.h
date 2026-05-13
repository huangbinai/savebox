#ifndef __BSP_UART_H
#define __BSP_UART_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bsp_platform.h"

#define LOG_DEBUG(huart, format, ...) \
    LOG_Debug_Out(huart, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

void UART_SendChar(UART_HandleTypeDef *huart, char ch);
void UART_SendString(UART_HandleTypeDef *huart, const char *msg);
void UART_Printf(UART_HandleTypeDef *huart, const char *format, ...);
int UART_ReadBytes(UART_HandleTypeDef *huart, uint8_t *data, uint32_t len, uint32_t timeout_ms);
int LOG_Debug_Out(UART_HandleTypeDef *huart, const char *__file, const char *__func, int __line, const char *format, ...);

#endif