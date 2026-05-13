#ifndef SAVEBOX_TIM_H
#define SAVEBOX_TIM_H

/*
 * STM32-style compatibility header.
 * Exposes the timer/PWM handles used by migrated app modules.
 */
#include "savebox_board.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

#endif
