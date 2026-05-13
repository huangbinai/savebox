#ifndef BSP_PLATFORM_H
#define BSP_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

typedef struct {
    int reserved;
} GPIO_TypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET = 1,
} GPIO_PinState;

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

typedef struct {
    ledc_mode_t speed_mode;
    ledc_timer_t timer_num;
    ledc_channel_t channel;
    gpio_num_t gpio_num;
    uint32_t frequency_hz;
    uint32_t period;
    bool pwm_started;
} TIM_HandleTypeDef;

typedef struct {
    spi_host_device_t host;
    spi_device_handle_t device;
    bool initialized;
} SPI_HandleTypeDef;

typedef struct {
    uart_port_t port;
    int tx_pin;
    int rx_pin;
    uint32_t baud_rate;
    bool use_driver;
    bool initialized;
} UART_HandleTypeDef;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

#define GPIO_MODE_INPUT 0x00U
#define GPIO_MODE_OUTPUT_PP 0x01U
#define GPIO_NOPULL 0x00U
#define GPIO_PULLUP 0x01U
#define GPIO_SPEED_FREQ_HIGH 0x00U

#define HAL_MAX_DELAY UINT32_MAX

#define TIM_CHANNEL_1 0x01U
#define TIM_CHANNEL_2 0x02U

esp_err_t savebox_platform_init(void);

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, const GPIO_InitTypeDef *GPIO_Init);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

void HAL_Delay(uint32_t ms);
uint32_t HAL_GetTick(void);

esp_err_t HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len, uint32_t timeout);
esp_err_t HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len, uint32_t timeout);
esp_err_t savebox_uart_driver_init(UART_HandleTypeDef *huart, int rx_buffer_size, int tx_buffer_size, int queue_size);
int savebox_uart_read(UART_HandleTypeDef *huart, uint8_t *data, uint32_t len, uint32_t timeout_ms);

esp_err_t savebox_pwm_start(TIM_HandleTypeDef *htim);
uint32_t savebox_pwm_get_period(const TIM_HandleTypeDef *htim);
uint32_t savebox_pwm_get_frequency(const TIM_HandleTypeDef *htim);
esp_err_t savebox_pwm_set_frequency(TIM_HandleTypeDef *htim, uint32_t freq_hz);
esp_err_t savebox_pwm_set_duty(TIM_HandleTypeDef *htim, float duty);

#endif