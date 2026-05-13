#include "app_Buzzer.h"

void buzzer(void)
{
    bsp_gpio_writePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
    delay_ms(50);
    bsp_gpio_writePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
}