#pragma once

#define VALID_ENTRY(x) (esp_timer_get_time() - x.last_time - x.first_time > 0)

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
    QueueHandle_t to_mqtt_queue;
};

bt_device_record_t device_history[BT_DEVICE_HISTORY_SIZE];

void bt_start(struct BTConf *conf);

void fast_timer_callback(void *arg);

void slow_timer_callback(void *arg);

void device_seen(char *name, int name_len, uint8_t *addr, int rssi);
