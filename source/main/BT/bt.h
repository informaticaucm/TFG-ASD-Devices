#pragma once
#include "esp_timer.h"

#define VALID_ENTRY(x) (esp_timer_get_time() - x.last_time - x.first_time > 0)



struct BTConf
{
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_mqtt_queue;
};

void bt_start(struct BTConf *conf);

void slow_timer_callback(void *arg);

void device_seen(char *name, int name_len, uint8_t *addr, int rssi);
