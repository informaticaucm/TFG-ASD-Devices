#pragma once

typedef struct
{
    char name[32];
    char address[6];
    int last_time;
    int first_time;
} bt_device_record_t;

struct BTConf
{
    QueueHandle_t to_screen_queue;
};

void bt_start(struct BTConf *conf);

void fast_timer_callback(void *arg);

void slow_timer_callback(void *arg);

void device_seen(char *name, int name_len, uint8_t *addr, int rssi);
