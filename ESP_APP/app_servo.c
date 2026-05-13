#include "app_servo.h"

void servo_Init(void)
{
    app_SetServo_Angle(90.0f);
}

void app_SetServo_Angle(float angle_to_set)
{
    if (angle_to_set < 0.0f) {
        angle_to_set = 0.0f;
    }
    if (angle_to_set > SERVO_ANGLE_LIMIT) {
        angle_to_set = SERVO_ANGLE_LIMIT;
    }

    bsp_set_pwm_Duty(&htim3, TIM_CHANNEL_1, (angle_to_set / SERVO_ANGLE_LIMIT * 10.0f + 2.5f) / 100.0f);
}

void app_lock(void)
{
    app_SetServo_Angle(90.0f);
}

void app_unlock(void)
{
    app_SetServo_Angle(10.0f);
}