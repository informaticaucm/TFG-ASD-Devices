#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct ButtonsConf
{
};

void buttons_start(struct ButtonsConf *conf);