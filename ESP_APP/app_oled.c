#include "app_oled.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define OLED_I2C_ADDRESS 0x78U
#define OLED_PAGE_COUNT 8U
#define OLED_PIXEL_WIDTH 128U
#define OLED_CHARS_PER_LINE (OLED_PIXEL_WIDTH / 8U)
#define OLED_GPIO_DELAY_US 4U
#define OLED_RESET_DELAY_MS 20U

typedef enum {
    OLED_CONTROL_COMMAND = 0x00U,
    OLED_CONTROL_DATA = 0x40U,
} oled_control_t;

static bool s_oled_initialized = false;

static bool oled_pin_valid(gpio_num_t pin)
{
    return ((int)pin >= 0) && ((int)pin < GPIO_NUM_MAX);
}

static void oled_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

static esp_err_t oled_config_open_drain(gpio_num_t pin)
{
    gpio_config_t config = {0};

    if (!oled_pin_valid(pin)) {
        return ESP_ERR_INVALID_ARG;
    }

    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&config);
}

static esp_err_t oled_config_output(gpio_num_t pin, int initial_level)
{
    gpio_config_t config = {0};

    if (!oled_pin_valid(pin)) {
        return ESP_OK;
    }

    config.pin_bit_mask = 1ULL << pin;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    return gpio_set_level(pin, initial_level);
}

static void oled_set_level(gpio_num_t pin, int level)
{
    if (oled_pin_valid(pin)) {
        gpio_set_level(pin, level);
    }
}

static void oled_sda_high(void)
{
    oled_set_level(SAVEBOX_OLED_SDA_GPIO, 1);
    oled_delay_us(OLED_GPIO_DELAY_US);
}

static void oled_sda_low(void)
{
    oled_set_level(SAVEBOX_OLED_SDA_GPIO, 0);
    oled_delay_us(OLED_GPIO_DELAY_US);
}

static void oled_scl_high(void)
{
    oled_set_level(SAVEBOX_OLED_SCL_GPIO, 1);
    oled_delay_us(OLED_GPIO_DELAY_US);
}

static void oled_scl_low(void)
{
    oled_set_level(SAVEBOX_OLED_SCL_GPIO, 0);
    oled_delay_us(OLED_GPIO_DELAY_US);
}

static void oled_i2c_start(void)
{
    oled_sda_high();
    oled_scl_high();
    oled_sda_low();
    oled_scl_low();
}

static void oled_i2c_stop(void)
{
    oled_sda_low();
    oled_scl_high();
    oled_sda_high();
}

static void oled_i2c_write_raw_byte(uint8_t data)
{
    uint8_t bit = 0U;

    for (bit = 0U; bit < 8U; ++bit) {
        if ((data & 0x80U) != 0U) {
            oled_sda_high();
        } else {
            oled_sda_low();
        }

        oled_scl_high();
        oled_scl_low();
        data <<= 1U;
    }

    oled_sda_high();
}

static void oled_i2c_skip_ack(void)
{
    oled_sda_high();
    oled_scl_high();
    oled_scl_low();
}

static void oled_write_packet(oled_control_t control, uint8_t value)
{
    oled_i2c_start();
    oled_i2c_write_raw_byte(OLED_I2C_ADDRESS);
    oled_i2c_skip_ack();
    oled_i2c_write_raw_byte((uint8_t)control);
    oled_i2c_skip_ack();
    oled_i2c_write_raw_byte(value);
    oled_i2c_skip_ack();
    oled_i2c_stop();
}

static void oled_reset_pulse(void)
{
    if (!oled_pin_valid(SAVEBOX_OLED_RESET_GPIO)) {
        return;
    }

    oled_set_level(SAVEBOX_OLED_RESET_GPIO, 0);
    HAL_Delay(OLED_RESET_DELAY_MS);
    oled_set_level(SAVEBOX_OLED_RESET_GPIO, 1);
    HAL_Delay(OLED_RESET_DELAY_MS);
}

static int Font8x8_GetIndex(char ch)
{
    if ((ch >= '0') && (ch <= '9')) {
        return ch - '0';
    }
    if ((ch >= 'A') && (ch <= 'Z')) {
        return 10 + (ch - 'A') * 2;
    }
    if ((ch >= 'a') && (ch <= 'z')) {
        return 11 + (ch - 'a') * 2;
    }
    if (ch == ',') {
        return 62;
    }
    if (ch == '.') {
        return 63;
    }
    if (ch == '=') {
        return 64;
    }
    if (ch == '+') {
        return 65;
    }
    if (ch == '-') {
        return 66;
    }
    if (ch == '!') {
        return 67;
    }
    if (ch == '*') {
        return 68;
    }
    if (ch == '(') {
        return 69;
    }
    if (ch == ')') {
        return 70;
    }
    if (ch == '/') {
        return 71;
    }
    if (ch == ':') {
        return 72;
    }

    return -1;
}

void OLED_Init(void)
{
    if (s_oled_initialized) {
        return;
    }

    if (oled_config_open_drain(SAVEBOX_OLED_SDA_GPIO) != ESP_OK) {
        return;
    }
    if (oled_config_open_drain(SAVEBOX_OLED_SCL_GPIO) != ESP_OK) {
        return;
    }
    if (oled_config_output(SAVEBOX_OLED_RESET_GPIO, 1) != ESP_OK) {
        return;
    }

    oled_sda_high();
    oled_scl_high();
    oled_reset_pulse();
    HAL_Delay(OLED_RESET_DELAY_MS);

    OLED_WriteByte(0xAE, OLED_CMD);
    OLED_WriteByte(0x00, OLED_CMD);
    OLED_WriteByte(0x10, OLED_CMD);
    OLED_WriteByte(0x40, OLED_CMD);
    OLED_WriteByte(0xB0, OLED_CMD);
    OLED_WriteByte(0x81, OLED_CMD);
    OLED_WriteByte(0xFF, OLED_CMD);
    OLED_WriteByte(0xA1, OLED_CMD);
    OLED_WriteByte(0xA6, OLED_CMD);
    OLED_WriteByte(0xA8, OLED_CMD);
    OLED_WriteByte(0x3F, OLED_CMD);
    OLED_WriteByte(0xC8, OLED_CMD);
    OLED_WriteByte(0xD3, OLED_CMD);
    OLED_WriteByte(0x00, OLED_CMD);
    OLED_WriteByte(0xD5, OLED_CMD);
    OLED_WriteByte(0x80, OLED_CMD);
    OLED_WriteByte(0xD9, OLED_CMD);
    OLED_WriteByte(0xF1, OLED_CMD);
    OLED_WriteByte(0xDA, OLED_CMD);
    OLED_WriteByte(0x12, OLED_CMD);
    OLED_WriteByte(0xDB, OLED_CMD);
    OLED_WriteByte(0x40, OLED_CMD);
    OLED_WriteByte(0x8D, OLED_CMD);
    OLED_WriteByte(0x14, OLED_CMD);
    OLED_WriteByte(0xAF, OLED_CMD);

    OLED_Clear();
    OLED_Set_Pos(0U, 0U);
    s_oled_initialized = true;
}

void OLED_WriteByte(uint8_t data, uint8_t cmd)
{
    oled_write_packet((cmd == OLED_CMD) ? OLED_CONTROL_COMMAND : OLED_CONTROL_DATA, data);
}

static void OLED_Fill(unsigned char fill_data)
{
    unsigned char page = 0U;
    unsigned char column = 0U;

    for (page = 0U; page < OLED_PAGE_COUNT; ++page) {
        OLED_WriteByte((uint8_t)(0xB0U + page), OLED_CMD);
        OLED_WriteByte(0x00U, OLED_CMD);
        OLED_WriteByte(0x10U, OLED_CMD);

        for (column = 0U; column < OLED_PIXEL_WIDTH; ++column) {
            OLED_WriteByte(fill_data, OLED_DATA);
        }
    }
}

void OLED_Clear(void)
{
    OLED_Fill(0x00U);
}

void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    OLED_WriteByte((uint8_t)(0xB0U + y), OLED_CMD);
    OLED_WriteByte((uint8_t)(x & 0x0FU), OLED_CMD);
    OLED_WriteByte((uint8_t)(((x & 0xF0U) >> 4U) | 0x10U), OLED_CMD);
}

void OLED_ShowChar8x8(uint8_t x, uint8_t y, char ch)
{
    int idx = Font8x8_GetIndex(ch);
    int col = 0;

    if (idx < 0) {
        return;
    }

    OLED_Set_Pos((unsigned char)(x * 8U), y);
    for (col = 7; col >= 0; --col) {
        uint8_t data = 0U;
        int row = 0;

        for (row = 0; row < 8; ++row) {
            if ((Font8x8[idx * 8 + row] & (1U << col)) != 0U) {
                data |= (uint8_t)(1U << row);
            }
        }

        OLED_WriteByte(data, OLED_DATA);
    }
}

void OLED_ShowString8x8(uint8_t x, uint8_t y, const char *str)
{
    if (str == NULL) {
        return;
    }

    while (*str != '\0') {
        OLED_ShowChar8x8(x, y, *str);
        x++;
        if (x >= OLED_CHARS_PER_LINE) {
            x = 0U;
            y++;
            if (y >= OLED_PAGE_COUNT) {
                break;
            }
        }
        str++;
    }
}

void OLED_ShowFormatString8x8(uint8_t x, uint8_t y, const char *format, ...)
{
    char buffer[64];
    va_list args;

    if (format == NULL) {
        return;
    }

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    OLED_ShowString8x8(x, y, buffer);
}