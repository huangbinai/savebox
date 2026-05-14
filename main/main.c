#include <inttypes.h>
#include <stdio.h>

#include "app_list.h"
#include "app_rc522.h"
#include "bsp_list.h"
#include "bsp_uart.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "savebox_mqtt.h"
#include "sdkconfig.h"
#include "task_list.h"
#include "task_rc522.h"
#include "task_state.h"

static void savebox_print_chip_info(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;

    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    printf("revision v%u.%u, ", chip_info.revision / 100, chip_info.revision % 100);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("flash size read failed\n");
        return;
    }

    printf("%" PRIu32 "MB flash\n", flash_size / (uint32_t)(1024 * 1024));
    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}

void app_main(void)
{
    printf("[boot_trace] before savebox_print_chip_info\n");
    savebox_print_chip_info();
    printf("[boot_trace] after savebox_print_chip_info\n");

    printf("[boot_trace] before BSP_Init\n");
    BSP_Init();
    printf("[boot_trace] after BSP_Init\n");

    printf("[boot_trace] before APP_Init\n");
    APP_Init();
    printf("[boot_trace] after APP_Init\n");

    printf("[boot_trace] before task_state_init\n");
    task_state_init();
    printf("[boot_trace] after task_state_init\n");

    printf("[boot_trace] before TASK_StartAll\n");
    TASK_StartAll();
    printf("[boot_trace] after TASK_StartAll\n");

    printf("[boot_trace] before savebox_mqtt_start\n");
    savebox_mqtt_start();
    printf("[boot_trace] after savebox_mqtt_start\n");

#if SAVEBOX_ENABLE_TASK_RC522 && SAVEBOX_ENABLE_TASK_RC522_DEFERRED_STARTUP
    printf("[boot_trace] deferred start task_rc522\n");
    task_rc522_start();
    printf("[boot_trace] deferred task_rc522 started\n");
#endif

    printf("ESP_APP + ESP_BSP migration active.\n");
    printf("Compatibility headers moved to /compat.\n");
    printf("Sensor and module tasks moved to /task.\n");
    printf("Pin mapping is defined in savebox_board.h.\n");

    UART_Printf(&huart1, "RC522 Version: 0x%02X\r\n", RC522_ReadRawRC(VersionReg));
    UART_Printf(&huart1, "UART command channel ready\r\n");
}
