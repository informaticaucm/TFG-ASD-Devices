#include <esp_log.h>
#include <inttypes.h>
#include "esp-idf-rc522/include/rc522.h"
#include "esp_gap_ble_api.h"

// Bluetooth specific includes
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
#include <esp_log.h>
#include "esp_timer.h"
#include <string.h>
#include <time.h>


#define PERSISTANCE_PERIOD 3

static const char *TAG = "rc522-demo";
static rc522_handle_t scanner;

char msg[20];

int scan_time = 0;

static void rc522_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    rc522_event_data_t *data = (rc522_event_data_t *)event_data;

    switch (event_id)
    {
    case RC522_EVENT_TAG_SCANNED:
    {
        scan_time = time(0);
        rc522_tag_t *tag = (rc522_tag_t *)data->ptr;
        ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);

        esp_ble_adv_data_t adv_data = {
            .set_scan_rsp = false,
            .include_name = true,
            .include_txpower = true,
            .min_interval = 0x0006,
            .max_interval = 0x0010,
            .appearance = 0x00,
            .manufacturer_len = sizeof(tag->serial_number),
            .p_manufacturer_data = (uint8_t *)&tag->serial_number,
            .service_data_len = 0,
            .p_service_data = NULL,
            .service_uuid_len = 32,
            .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
        };

        esp_ble_gap_config_adv_data(&adv_data);
    }
    break;
    }
}

#include "nvs.h"
#include "nvs_flash.h"

void periodic_task(){
    ESP_LOGE(TAG, "periodic task");
    if (time(0) - scan_time > PERSISTANCE_PERIOD){
        esp_ble_adv_data_t adv_data = {
            .set_scan_rsp = false,
            .include_name = true,
            .include_txpower = true,
            .min_interval = 0x0006,
            .max_interval = 0x0010,
            .appearance = 0x00,
            .manufacturer_len = 0,
            .p_manufacturer_data = (uint8_t *)"",
            .service_data_len = 0,
            .p_service_data = NULL,
            .service_uuid_len = 32,
            .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
        };
        esp_ble_gap_config_adv_data(&adv_data);
    }
}

void app_main()
{
    ESP_LOGE(TAG, "initing flash");
    esp_err_t err = nvs_flash_init();
    ESP_LOGE(TAG, "initing flash finished with error %s", esp_err_to_name(err));

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 26,
        .spi.mosi_gpio = 27,
        .spi.sck_gpio = 14,
        .spi.sda_gpio = 12,
    };

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);

    // Initialize BT controller to allocate task and other resource.
    ESP_LOGI(TAG, "Enabling Bluetooth Controller");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&bt_cfg) != ESP_OK)
    {
        ESP_LOGE(TAG, "Bluetooth controller initialize failed");
    }

    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = 0,                // TEST_MANUFACTURER_DATA_LEN,
        .p_manufacturer_data = (uint8_t *)"", //&test_manufacturer[0],
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 32,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    esp_ble_gap_set_device_name("RC522-READER");

    esp_ble_gap_config_adv_data(&adv_data);

    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_NONCONN_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    esp_ble_gap_start_advertising(&adv_params);

    {
        const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_task,
            .name = "periodic",
            .arg = NULL};

        /* Create timer for logging scanned devices. */
        esp_timer_handle_t periodic_timer;
        ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

        /* Start periodic timer for 5 sec. */
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, PERSISTANCE_PERIOD * 2 * 1000000)); // 2 * PERSISTANCE_PERIOD sec
    }
}