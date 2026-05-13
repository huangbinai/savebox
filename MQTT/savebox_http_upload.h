#ifndef SAVEBOX_HTTP_UPLOAD_H
#define SAVEBOX_HTTP_UPLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t savebox_http_upload_latest_frame(const uint8_t *data, size_t len, uint32_t captured_at_ms);
esp_err_t savebox_http_upload_alarm_frame(const uint8_t *data,
                                          size_t len,
                                          const char *alarm_type,
                                          uint32_t captured_at_ms);
esp_err_t savebox_http_upload_status_snapshot(void);

#endif
