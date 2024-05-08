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

int get_disposable_record_i(struct bt_device_record *device_history)
{
    int oldest_record = 0;
    int oldest_time = time(0);
    // find the record with the oldest time
    for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
    {
        if (!VALID_ENTRY(device_history[i]))
        {
            ESP_LOGE(TAG, "found invalid record at %d", i);
            return i;
        }
        if (device_history[i].last_time < oldest_time)
        {
            oldest_time = device_history[i].last_time;
            oldest_record = i;
        }
    }
    ESP_LOGE(TAG, "found oldest record at %d", oldest_record);

    return oldest_record;
}

void slow_timer_callback(void *arg)
{

    struct BTConf *conf = (struct BTConf *)arg;

    struct ConnectionParameters parameters;
    j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters));

    jsend(conf->to_mqtt_queue, MQTTMsg, {
        msg->command = FetchBTMacs;
        msg->data.fetch_btmacs.space_id = parameters.qr_info.space_id;
    });
}

void device_seen(char *scanned_name, uint8_t *addr, int rssi)
{
    struct bt_device_record device_history[BT_DEVICE_HISTORY_SIZE];
    get_bt_device_history(device_history);

    // ESP_LOGE(TAG, "device seen %s", scanned_name);

    bool found = false;
    // check if device is already in history
    int j = 0;
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
        // ESP_LOGI(TAG, "updating record %d", j);

        // update the time
        device_history[j].last_time = time(0);
    }
    else
    {
        int overwriting_record = get_disposable_record_i(device_history);

        // ESP_LOGI(TAG, "overwriting record %d %s", overwriting_record, scanned_name);

        /* Store the scanned device in history. */
        device_history[overwriting_record].last_time = time(0);
        device_history[overwriting_record].first_time = time(0);
        device_history[overwriting_record].valid = true;
        strncpy(device_history[overwriting_record].name, scanned_name, sizeof(device_history[overwriting_record].name));
        memcpy(device_history[overwriting_record].address, addr, 6);
    }

    // print the history

    // for (int i = 0; i < BT_DEVICE_HISTORY_SIZE; i++)
    // {
    //     if (VALID_ENTRY(device_history[i]))
    //     {
    //         ESP_LOGE("historial_bt", "%d: device %s %d", i, device_history[i].name, (int)time(0) - device_history[i].last_time);
    //     }
    //     else
    //     {
    //         ESP_LOGE("historial_bt", "%d: -----", i);
    //     }
    // }

    set_bt_device_history(device_history);
}

uint64_t last_sn = 0;

void rfid_seen(uint64_t sn, int rssi, struct BTConf *conf)
{

    if (sn != last_sn)
    {
        if (sn != 0)
        {
            ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ") con rssi: %d", sn, rssi);

            {
                jsend(conf->to_screen_queue, ScreenMsg, {
                    msg->command = ShowMsg;
                    snprintf(msg->data.text, sizeof(msg->data.text), "tarjeta leida");
                });
            }

            struct ConnectionParameters parameters;
            ESP_ERROR_CHECK(j_nvs_get(nvs_conf_tag, &parameters, sizeof(struct ConnectionParameters)));

            jsend(conf->to_mqtt_queue, MQTTMsg, {
                msg->command = TagScanned;
                msg->data.tag_scanned.sn = sn;
            });
        }
        last_sn = sn;
    }
}