#include "task_state.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    savebox_dht11_state_t dht11;
    savebox_mq2_state_t mq2;
    savebox_vibration_state_t vibration;
    savebox_card_state_t card;
    savebox_lock_state_t lock;
    savebox_camera_state_t camera;
} savebox_runtime_state_t;

static SemaphoreHandle_t s_state_mutex = NULL;
static savebox_runtime_state_t s_state = {
    .dht11 = {0},
    .mq2 = {0},
    .vibration = {0},
    .card = {0},
    .lock = {
        .locked = true,
    },
};

static bool task_state_take(void)
{
    return (s_state_mutex != NULL) && (xSemaphoreTake(s_state_mutex, portMAX_DELAY) == pdTRUE);
}

static void task_state_give(void)
{
    if (s_state_mutex != NULL) {
        xSemaphoreGive(s_state_mutex);
    }
}

void task_state_init(void)
{
    if (s_state_mutex != NULL) {
        return;
    }

    s_state_mutex = xSemaphoreCreateMutex();
}

void task_state_set_dht11(uint8_t temp_int,
                          uint8_t temp_dec,
                          uint8_t humi_int,
                          uint8_t humi_dec,
                          uint32_t updated_at_ms)
{
    if (!task_state_take()) {
        return;
    }

    s_state.dht11.valid = true;
    s_state.dht11.temp_int = temp_int;
    s_state.dht11.temp_dec = temp_dec;
    s_state.dht11.humi_int = humi_int;
    s_state.dht11.humi_dec = humi_dec;
    s_state.dht11.updated_at_ms = updated_at_ms;
    task_state_give();
}

void task_state_get_dht11(savebox_dht11_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.dht11;
    task_state_give();
}

void task_state_set_mq2(int raw,
                        int voltage_mv,
                        int delta_raw,
                        bool warmup_active,
                        uint32_t warmup_remaining_ms,
                        bool alarm,
                        uint32_t updated_at_ms)
{
    if (!task_state_take()) {
        return;
    }

    s_state.mq2.valid = true;
    s_state.mq2.raw = raw;
    s_state.mq2.voltage_mv = voltage_mv;
    s_state.mq2.delta_raw = delta_raw;
    s_state.mq2.warmup_active = warmup_active;
    s_state.mq2.warmup_remaining_ms = warmup_remaining_ms;
    s_state.mq2.alarm = alarm;
    s_state.mq2.updated_at_ms = updated_at_ms;
    task_state_give();
}

void task_state_get_mq2(savebox_mq2_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.mq2;
    task_state_give();
}

void task_state_set_vibration(int level, bool alarm)
{
    if (!task_state_take()) {
        return;
    }

    s_state.vibration.valid = true;
    s_state.vibration.level = level;
    s_state.vibration.alarm = alarm;
    task_state_give();
}

void task_state_get_vibration(savebox_vibration_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.vibration;
    task_state_give();
}

void task_state_set_card(const uint8_t uid[4], uint32_t updated_at_ms)
{
    if ((uid == NULL) || !task_state_take()) {
        return;
    }

    s_state.card.valid = true;
    memcpy(s_state.card.uid, uid, sizeof(s_state.card.uid));
    s_state.card.updated_at_ms = updated_at_ms;
    task_state_give();
}

void task_state_clear_card(void)
{
    if (!task_state_take()) {
        return;
    }

    memset(&s_state.card, 0, sizeof(s_state.card));
    task_state_give();
}

void task_state_get_card(savebox_card_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.card;
    task_state_give();
}

void task_state_set_lock(bool locked, uint32_t updated_at_ms)
{
    if (!task_state_take()) {
        return;
    }

    s_state.lock.locked = locked;
    s_state.lock.updated_at_ms = updated_at_ms;
    task_state_give();
}

void task_state_get_lock(savebox_lock_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.lock;
    task_state_give();
}

void task_state_set_camera(bool initialized,
                           bool last_capture_ok,
                           bool last_upload_ok,
                           bool last_alarm_upload_ok,
                           uint32_t last_capture_ms,
                           uint32_t last_upload_ms,
                           uint32_t last_alarm_upload_ms)
{
    if (!task_state_take()) {
        return;
    }

    s_state.camera.initialized = initialized;
    s_state.camera.last_capture_ok = last_capture_ok;
    s_state.camera.last_upload_ok = last_upload_ok;
    s_state.camera.last_alarm_upload_ok = last_alarm_upload_ok;
    s_state.camera.last_capture_ms = last_capture_ms;
    s_state.camera.last_upload_ms = last_upload_ms;
    s_state.camera.last_alarm_upload_ms = last_alarm_upload_ms;
    task_state_give();
}

void task_state_get_camera(savebox_camera_state_t *state)
{
    if ((state == NULL) || !task_state_take()) {
        return;
    }

    *state = s_state.camera;
    task_state_give();
}
