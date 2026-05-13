#ifndef __TASK_SERVO_H
#define __TASK_SERVO_H

#include <stdbool.h>

void task_servo_start(void);
bool task_servo_set_angle(float angle);
bool task_servo_lock(void);
bool task_servo_unlock(void);

#endif
