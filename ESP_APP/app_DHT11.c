#include "app_DHT11.h"

static void DHT11_Set_Pin_Input(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = DHT11_DATA_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &gpio_init);
}

static void DHT11_Set_Pin_Output(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = DHT11_DATA_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &gpio_init);
}

static uint8_t DHT11_Read_Byte(uint8_t *data)
{
    uint8_t i = 0;
    uint8_t byte = 0;
    uint16_t timeout = 0;

    for (i = 0; i < 8; i++) {
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_RESET) {
            if (++timeout > DHT11_TIMEOUT) {
                return 1;
            }
            delay_us(1);
        }

        delay_us(40);
        if (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET) {
            byte |= (1U << (7U - i));
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET) {
                if (++timeout > DHT11_TIMEOUT) {
                    return 1;
                }
                delay_us(1);
            }
        }
    }

    *data = byte;
    return 0;
}

uint8_t DHT_Read(uint8_t *temp_int, uint8_t *temp_dec, uint8_t *humi_int, uint8_t *humi_dec)
{
    uint8_t data[5] = {0};
    uint8_t checksum = 0;
    uint16_t timeout = 0;
    int i = 0;

    DHT11_Set_Pin_Output();
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);
    DHT11_Set_Pin_Input();

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET) {
        if (++timeout > DHT11_TIMEOUT) {
            return 1;
        }
        delay_us(1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_RESET) {
        if (++timeout > DHT11_TIMEOUT) {
            return 2;
        }
        delay_us(1);
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET) {
        if (++timeout > DHT11_TIMEOUT) {
            return 3;
        }
        delay_us(1);
    }

    for (i = 0; i < 5; i++) {
        if (DHT11_Read_Byte(&data[i]) != 0U) {
            return 4;
        }
    }

    checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        return 5;
    }

    *humi_int = data[0];
    *humi_dec = data[1];
    *temp_int = data[2];
    *temp_dec = data[3];
    return 0;
}