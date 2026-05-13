#ifndef __TASK_CAMERA_H
#define __TASK_CAMERA_H

#include <stdbool.h>

typedef enum {
    SAVEBOX_CAMERA_ALARM_NONE = 0,
    SAVEBOX_CAMERA_ALARM_GAS = (1 << 0),
    SAVEBOX_CAMERA_ALARM_VIBRATION = (1 << 1),
} savebox_camera_alarm_t;

void task_camera_start(void);
bool task_camera_notify_alarm(savebox_camera_alarm_t alarm_type);

#endif
