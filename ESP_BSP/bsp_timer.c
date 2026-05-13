#include "bsp_timer.h"

void bsp_timer_Init(void)
{
    savebox_pwm_start(&htim3);
}

uint32_t bsp_get_pwm_Period(TIM_HandleTypeDef *htim)
{
    return savebox_pwm_get_period(htim);
}

uint32_t bsp_get_pwm_Freq(TIM_HandleTypeDef *htim, uint32_t timer_clk_freq)
{
    (void)timer_clk_freq;
    return savebox_pwm_get_frequency(htim);
}

void bsp_set_pwm_Freq(TIM_HandleTypeDef *htim, uint32_t timer_clk_freq, uint32_t freq_to_set)
{
    (void)timer_clk_freq;

    if (freq_to_set == 0U) {
        return;
    }

    savebox_pwm_set_frequency(htim, freq_to_set);
}

void bsp_set_pwm_Duty(TIM_HandleTypeDef *htim, uint32_t Channel, float duty)
{
    (void)Channel;

    if (duty < 0.0f) {
        duty = 0.0f;
    }
    if (duty > 1.0f) {
        duty = 1.0f;
    }

    savebox_pwm_set_duty(htim, duty);
}