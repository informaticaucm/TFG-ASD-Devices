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
#include "common.h"

#define TAG "BT_LOGIC"

#define MAX_DISTANCE_BETWEEN_SCANS 60 * 10 // 10 minutes
#define VALID_ENTRY(x) (esp_timer_get_time() - x.last_time - x.first_time > 0)

bt_device_record_t device_history[BT_DEVICE_HISTORY_SIZE];

void fast_timer_callback(void *arg)
{
    struct BTConf *conf = (struct BTConf *)arg;

    struct ScreenMsg *msg = (struct ScreenMsg *)jalloc(sizeof(struct ScreenMsg));
    msg->command = BTUpdate;
    memcpy(msg->data.bt_devices, device_history, BT_DEVICE_HISTORY_SIZE * sizeof(bt_device_record_t));

    int res = xQueueSend(conf->to_screen_queue, &msg, 0);
    if (res != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to send BTUpdate to screen queue");
        free(msg);
    }
}

void slow_timer_callback(void *arg)
{

}

void device_seen(char* scanned_name, int name_len, uint8_t* addr, int rssi)
{
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
        memcpy(device_history[oldest_record].name, scanned_name, name_len);
        device_history[oldest_record].name[name_len] = '\0';
        memcpy(device_history[oldest_record].address, addr, 6);
    }

    // sort the history by last_time 
    for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
    {
        for (int j = i + 1; j < BT_DEVICE_HISTORY_SIZE; j++)
        {
            if (device_history[i].first_time < device_history[j].first_time)
            {
                bt_device_record_t temp = device_history[i];
                device_history[i] = device_history[j];
                device_history[j] = temp;
            }
        }
    }
}