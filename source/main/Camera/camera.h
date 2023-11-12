#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_camera.h"

enum CameraCommand
{
    StreamToScreen
};

struct CameraMsg
{
    enum CameraCommand command;
    union
    {
        struct
        {
            int time;
            int refreshRate;
        } stream;
    } data;
};

struct CameraConf
{
    QueueHandle_t to_qr_queue;
    QueueHandle_t to_cam_queue;
    QueueHandle_t to_screen_queue;
    camera_config_t *camera_config;
};

void camera_start(struct CameraConf *conf);