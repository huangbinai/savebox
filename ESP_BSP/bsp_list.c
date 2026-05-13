#include "bsp_list.h"

#include "app_oled.h"
#include "savebox_board.h"

void BSP_Init(void)
{
    printf("[bsp_trace] BSP_Init enter\n");

#if SAVEBOX_CAMERA_ONLY_ISOLATION
    printf("[bsp_trace] skip savebox_platform_init for full camera isolation\n");
    printf("[bsp_trace] skip savebox_uart_driver_init for boot isolation\n");
#else
#if SAVEBOX_ENABLE_BSP_PLATFORM
    printf("[bsp_trace] before savebox_platform_init\n");
    if (savebox_platform_init() != ESP_OK) {
        printf("[bsp_trace] savebox_platform_init failed\n");
    }
    printf("[bsp_trace] after savebox_platform_init\n");
#else
    printf("[bsp_trace] savebox_platform_init disabled\n");
#endif

#if SAVEBOX_ENABLE_BSP_UART_DRIVER
    printf("[bsp_trace] before savebox_uart_driver_init\n");
    if (savebox_uart_driver_init(&huart1, 256, 0, 0) != ESP_OK) {
        printf("[bsp_trace] savebox_uart_driver_init failed\n");
    }
    printf("[bsp_trace] after savebox_uart_driver_init\n");
#else
    printf("[bsp_trace] savebox_uart_driver_init disabled\n");
#endif
#endif

    printf("[bsp_trace] before bsp_timer_Init\n");
    bsp_timer_Init();
    printf("[bsp_trace] after bsp_timer_Init\n");

#if SAVEBOX_OLED_AUTO_INIT
    printf("[bsp_trace] before OLED_Init\n");
    OLED_Init();
    printf("[bsp_trace] after OLED_Init\n");
#endif

    printf("[bsp_trace] BSP_Init leave\n");
}
