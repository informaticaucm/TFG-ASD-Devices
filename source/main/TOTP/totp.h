#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct TOTPConf
{
    QueueHandle_t to_screen_queue;
};

void start_totp(struct TOTPConf *conf);