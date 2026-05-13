#ifndef SAVEBOX_SPI_H
#define SAVEBOX_SPI_H

/*
 * STM32-style compatibility header.
 * Exposes the SPI handle used by the migrated RC522 driver.
 */
#include "savebox_board.h"

extern SPI_HandleTypeDef hspi1;

#endif
