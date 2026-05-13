#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#include "bsp_platform.h"

void bsp_timer_Init(void);
uint32_t bsp_get_pwm_Period(TIM_HandleTypeDef *htim);
uint32_t bsp_get_pwm_Freq(TIM_HandleTypeDef *htim, uint32_t timer_clk_freq);
void bsp_set_pwm_Freq(TIM_HandleTypeDef *htim, uint32_t timer_clk_freq, uint32_t freq_to_set);
void bsp_set_pwm_Duty(TIM_HandleTypeDef *htim, uint32_t Channel, float duty);

#endif