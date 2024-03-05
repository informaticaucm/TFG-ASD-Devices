#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct BTConf
{
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_mqtt_queue;
};

void bt_start(struct BTConf *conf);

void slow_timer_callback(void *arg);

void device_seen(char *name, uint8_t *addr, int rssi);
void rfid_seen(uint64_t sn, int rssi);
