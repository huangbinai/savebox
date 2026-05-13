#include "app_sw180.h"

#include "savebox_board.h"

esp_err_t SW180_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = SW180_DO_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SW180_DO_GPIO_Port, &gpio_init);



    return ESP_OK;
}

GPIO_PinState SW180_ReadDO(void)
{
    return HAL_GPIO_ReadPin(SW180_DO_GPIO_Port, SW180_DO_Pin);
}

bool SW180_IsTriggered(void)
{
    return (SW180_ReadDO() == SAVEBOX_SW180_TRIGGER_LEVEL);
}