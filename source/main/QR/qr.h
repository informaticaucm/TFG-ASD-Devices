#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct QRConf
{
    QueueHandle_t cam_to_qr_queue;
    QueueHandle_t qr_to_mqtt_queue;
    QueueHandle_t qr_to_starter_queue;
};

void qr_start(QRConf conf);
