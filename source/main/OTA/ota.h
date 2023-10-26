#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct OTAMsg
{
    char *url;
};

struct OTAConf
{
    QueueHandle_t ota_to_mqtt_queue;
    QueueHandle_t mqtt_to_ota_queue;
    QueueHandle_t ota_to_screen_queue;
};

void ota_start(OTAConf conf);