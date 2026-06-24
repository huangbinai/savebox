#ifndef __APP_OLED_H
#define __APP_OLED_H

#include <stdint.h>

#include "OLED_font.h"
#include "savebox_board.h"

#define OLED_CMD 1U
#define OLED_DATA 0U
#define SIZE 8U
#define Max_Column (128U / SIZE)

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearLine(uint8_t y);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_WriteByte(uint8_t data, uint8_t cmd);
void OLED_ShowChar8x8(uint8_t x, uint8_t y, char ch);
void OLED_ShowString8x8(uint8_t x, uint8_t y, const char *str);
void OLED_ShowFormatString8x8(uint8_t x, uint8_t y, const char *format, ...);

#endif
