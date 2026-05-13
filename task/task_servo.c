#include "task_servo.h"

#include "app_servo.h"
#include "bsp_platform.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "task_state.h"

#define TASK_SERVO_STACK_SIZE 2048
#define TASK_SERVO_PRIORITY 3
#define TASK_SERVO_QUEUE_LENGTH 4

typedef struct {
    float angle;
} servo_command_t;

static QueueHandle_t s_servo_queue = NULL;

static void task_servo_entry(void *arg)
{
    servo_command_t command = {0};

    (void)arg;

    app_lock();
    task_state_set_lock(true, HAL_GetTick());

    for (;;) {
        if (xQueueReceive(s_servo_queue, &command, portMAX_DELAY) == pdTRUE) {
            app_SetServo_Angle(command.angle);
        }
    }
}

void task_servo_start(void)
{
    if (s_servo_queue != NULL) {
        return;
    }

    s_servo_queue = xQueueCreate(TASK_SERVO_QUEUE_LENGTH, sizeof(servo_command_t));
    if (s_servo_queue == NULL) {
        return;
    }

    xTaskCreate(task_servo_entry, "task_servo", TASK_SERVO_STACK_SIZE, NULL, TASK_SERVO_PRIORITY, NULL);
}

bool task_servo_set_angle(float angle)
{
    servo_command_t command = {
        .angle = angle,
    };

    if (s_servo_queue == NULL) {
        return false;
    }

    return xQueueSend(s_servo_queue, &command, 0) == pdTRUE;
}

bool task_servo_lock(void)
{
    const bool ok = task_servo_set_angle(90.0f);
    if (ok) {
        task_state_set_lock(true, HAL_GetTick());
    }
    return ok;
}

bool task_servo_unlock(void)
{
    const bool ok = task_servo_set_angle(10.0f);
    if (ok) {
        task_state_set_lock(false, HAL_GetTick());
    }
    return ok;
}