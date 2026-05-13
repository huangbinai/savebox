#include "task_list.h"

#include "task_buzzer.h"
#include "task_camera.h"
#include "task_dht11.h"
#include "task_mq2.h"
#include "task_oled.h"
#include "task_rc522.h"
#include "task_servo.h"
#include "task_sw180.h"
#include "task_uart.h"
#include "savebox_board.h"

void TASK_StartAll(void)
{
#if SAVEBOX_ENABLE_TASK_CAMERA
    task_camera_start();
#endif

#if !SAVEBOX_CAMERA_ONLY_ISOLATION
#if SAVEBOX_ENABLE_TASK_BUZZER
    task_buzzer_start();
#endif
#if SAVEBOX_ENABLE_TASK_SERVO
    task_servo_start();
#endif
#if SAVEBOX_ENABLE_TASK_UART
    task_uart_start();
#endif
#if SAVEBOX_ENABLE_TASK_DHT11
    task_dht11_start();
#endif
#if SAVEBOX_ENABLE_TASK_RC522
    task_rc522_start();
#endif
#if SAVEBOX_ENABLE_TASK_MQ2
    task_mq2_start();
#endif
#if SAVEBOX_ENABLE_TASK_SW180
    task_sw180_start();
#endif
#if SAVEBOX_ENABLE_TASK_OLED
    task_oled_start();
#endif
#endif
}
