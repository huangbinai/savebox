#ifndef __TASK_BUZZER_H
#define __TASK_BUZZER_H

#include <stdbool.h>
#include <stdint.h>

void task_buzzer_start(void);
bool task_buzzer_beep(uint32_t duration_ms);

#endif
