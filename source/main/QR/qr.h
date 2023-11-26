#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "quirc.h"
#include "quirc_internal.h"

struct QRConf
{
    QueueHandle_t to_qr_queue;
    QueueHandle_t to_mqtt_queue;
    QueueHandle_t to_starter_queue;
    struct quirc *qr;
};

void qr_start(struct QRConf *conf);
