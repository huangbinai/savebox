#ifndef __TASK_STATE_H
#define __TASK_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool valid;
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t humi_int;
    uint8_t humi_dec;
    uint32_t updated_at_ms;
} savebox_dht11_state_t;

typedef struct {
    bool valid;
    int raw;
    int voltage_mv;
    int delta_raw;
    uint32_t warmup_remaining_ms;
    bool warmup_active;
    bool alarm;
    uint32_t updated_at_ms;
} savebox_mq2_state_t;

typedef struct {
    bool valid;
    bool alarm;
    int level;
    uint32_t updated_at_ms;
} savebox_vibration_state_t;

typedef struct {
    bool valid;
    uint8_t uid[4];
    uint32_t updated_at_ms;
} savebox_card_state_t;

typedef struct {
    bool locked;
    uint32_t updated_at_ms;
} savebox_lock_state_t;

typedef struct {
    bool initialized;
    bool last_capture_ok;
    bool last_upload_ok;
    bool last_alarm_upload_ok;
    uint32_t last_capture_ms;
    uint32_t last_upload_ms;
    uint32_t last_alarm_upload_ms;
} savebox_camera_state_t;

void task_state_init(void);

void task_state_set_dht11(uint8_t temp_int,
                          uint8_t temp_dec,
                          uint8_t humi_int,
                          uint8_t humi_dec,
                          uint32_t updated_at_ms);
void task_state_get_dht11(savebox_dht11_state_t *state);

void task_state_set_mq2(int raw,
                        int voltage_mv,
                        int delta_raw,
                        bool warmup_active,
                        uint32_t warmup_remaining_ms,
                        bool alarm,
                        uint32_t updated_at_ms);
void task_state_get_mq2(savebox_mq2_state_t *state);

void task_state_set_vibration(int level, bool alarm);
void task_state_get_vibration(savebox_vibration_state_t *state);

void task_state_set_card(const uint8_t uid[4], uint32_t updated_at_ms);
void task_state_clear_card(void);
void task_state_get_card(savebox_card_state_t *state);

void task_state_set_lock(bool locked, uint32_t updated_at_ms);
void task_state_get_lock(savebox_lock_state_t *state);

void task_state_set_camera(bool initialized,
                           bool last_capture_ok,
                           bool last_upload_ok,
                           bool last_alarm_upload_ok,
                           uint32_t last_capture_ms,
                           uint32_t last_upload_ms,
                           uint32_t last_alarm_upload_ms);
void task_state_get_camera(savebox_camera_state_t *state);

#endif
