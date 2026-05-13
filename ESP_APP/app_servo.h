#ifndef __APP_SERVO_H
#define __APP_SERVO_H

#include "compat/tim.h"
#include "bsp_timer.h"

#define SERVO_ANGLE_LIMIT 180.0f

void servo_Init(void);
void app_SetServo_Angle(float angle_to_set);
void app_lock(void);
void app_unlock(void);

#endif
