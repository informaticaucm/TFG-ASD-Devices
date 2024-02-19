#pragma once

typedef struct
{
    char name[32];
    char address[6];
    int time;
} bt_device_record_t;

struct BTConf{
    QueueHandle_t to_screen_queue;
};

void bt_start(struct BTConf *conf);

