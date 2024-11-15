#ifndef __OTA_H__
#define __OTA_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"
#include "esp_camera.h"

enum OTACommand
{
    Update,
    CancelRollback
};


struct OTAMsg
{
    enum OTACommand command;
    bool requested;
    char url[OTA_URL_SIZE];
};

struct OTAConf
{
    QueueHandle_t to_mqtt_queue;
    QueueHandle_t to_ota_queue;
    QueueHandle_t to_screen_queue;
    // camera_config_t *cam_conf;
};

void ota_start(struct OTAConf *conf);

#endif