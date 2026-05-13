#ifndef SAVEBOX_MQTT_H
#define SAVEBOX_MQTT_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t savebox_mqtt_start(void);
esp_err_t savebox_mqtt_publish_snapshot(void);
bool savebox_mqtt_is_connected(void);

#endif
