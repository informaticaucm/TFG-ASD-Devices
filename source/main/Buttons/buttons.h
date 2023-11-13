#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct ButtonsConf
{
    QueueHandle_t to_cam_queue;
};

void buttons_start(struct ButtonsConf *conf);