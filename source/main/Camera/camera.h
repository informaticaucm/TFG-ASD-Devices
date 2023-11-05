#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_camera.h"

struct CameraConf
{
    QueueHandle_t cam_to_qr_queue;
    camera_config_t *camera_config;
};

void camera_start(struct CameraConf *conf);