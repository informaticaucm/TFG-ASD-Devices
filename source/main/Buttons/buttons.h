#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct ButtonsConf
{
};

void buttons_start(struct ButtonsConf *conf);

#include "esp_timer.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_log.h"

