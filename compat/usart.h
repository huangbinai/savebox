#ifndef SAVEBOX_USART_H
#define SAVEBOX_USART_H

/*
 * STM32-style compatibility header.
 * Exposes the UART handle used by migrated logging code.
 */
#include "savebox_board.h"

extern UART_HandleTypeDef huart1;

#endif
