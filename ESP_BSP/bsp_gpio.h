#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "bsp_platform.h"

GPIO_PinState bsp_gpio_readPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void bsp_gpio_writePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void bsp_gpio_togglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#endif