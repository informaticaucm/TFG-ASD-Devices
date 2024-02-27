#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/queue.h"
#include "bt_hci_common.h"

#include "../SYS_MODE/sys_mode.h"
#include "bt.h"
#include "../Screen/screen.h"
#include "../MQTT/mqtt.h"
#include "common.h"
#include "nvs_plugin.h"

#define TAG "BT_LOGIC"

#define MAX_DISTANCE_BETWEEN_SCANS 60 * 30 // 10 minutes

void slow_timer_callback(void *arg)
{

    struct ConnectionParameters parameters;
    int err = j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    if (get_last_ping_time() - esp_timer_get_time() > PING_RATE * 3)
    {
        return;
    }

    struct BTConf *conf = (struct BTConf *)arg;
    struct MQTTMsg *msg = jalloc(sizeof(struct MQTTMsg));

    msg->command = FetchBTMacs;
    msg->data.fetch_btmacs.space_id = parameters.qr_info.space_id;

    int res = xQueueSend(conf->to_mqtt_queue, &msg, 0);
}

void device_seen(char *scanned_name, uint8_t *addr, int rssi)
{
    struct bt_device_record device_history[BT_DEVICE_HISTORY_SIZE];
    get_bt_device_history(device_history);

    ESP_LOGE(TAG, "device seen %s", scanned_name);

    bool found = false;
    // check if device is already in history
    int j = 0;
    int now = esp_timer_get_time();
    for (; j < BT_DEVICE_HISTORY_SIZE; j++)
    {
        if (memcmp(device_history[j].address, addr, 6) == 0 && VALID_ENTRY(device_history[j]))
        {
            found = true;
            break;
        }
    }

    if (found)
    {
        // update the time
        device_history[j].last_time = now;
    }
    else
    {
        int oldest_record = 0;
        int oldest_time = now;
        // find the record with the oldest time
        for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
        {
            if (device_history[i].last_time < oldest_time)
            {
                oldest_time = device_history[i].last_time;
                oldest_record = i;
            }
        }

        /* Store the scanned device in history. */
        device_history[oldest_record].last_time = now;
        device_history[oldest_record].first_time = now;
        strcpy(device_history[oldest_record].name, scanned_name);
        memcpy(device_history[oldest_record].address, addr, 6);
    }

    // sort the history by last_time
    for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
    {
        for (int j = i + 1; j < BT_DEVICE_HISTORY_SIZE; j++)
        {
            if (device_history[i].first_time < device_history[j].first_time)
            {
                struct bt_device_record temp = device_history[i];
                device_history[i] = device_history[j];
                device_history[j] = temp;
            }
        }
    }

    set_bt_device_history(device_history);
}