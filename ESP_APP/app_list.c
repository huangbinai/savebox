#include "app_list.h"
#include "savebox_board.h"

void APP_Init(void)
{
#if SAVEBOX_ENABLE_APP_CAMERA
    savebox_camera_init();
#endif

#if SAVEBOX_CAMERA_ONLY_ISOLATION
    printf("[app_trace] camera-only APP_Init for isolation\n");
#else
#if SAVEBOX_ENABLE_APP_SERVO
    servo_Init();
#endif
#if SAVEBOX_ENABLE_APP_SW180
    SW180_Init();
#endif
#if SAVEBOX_ENABLE_APP_RC522
    RC522_Init();
#endif
#if SAVEBOX_ENABLE_APP_MQ2
    MQ2_Init();
#endif
    printf("[app_trace] full APP_Init\n");
#endif
}
