#ifndef __APP_CAMERA_H
#define __APP_CAMERA_H

#include <stdbool.h>

#include "esp_camera.h"
#include "esp_err.h"

esp_err_t savebox_camera_init(void);
bool savebox_camera_is_ready(void);
camera_fb_t *savebox_camera_capture_jpeg(void);
void savebox_camera_return_frame(camera_fb_t *fb);

#endif
