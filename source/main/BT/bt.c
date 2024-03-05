#include "bt.h"
static const char *TAG = "BT_BACKEND";
#define CONFIG_EXAMPLE_EXTENDED_ADV 1

// #include <esp_bt.h>
// #include <esp_bt_main.h>
// #include <esp_gap_ble_api.h>
// #include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
// #include <esp_log.h>
// #include "esp_timer.h"

// #include "esp_bt.h"
// #include "esp_bt_main.h"
// #include "esp_gap_ble_api.h"

// #include "string.h"

// // scan parameters
// static esp_ble_scan_params_t ble_scan_params = {
//     .scan_type = BLE_SCAN_TYPE_ACTIVE,
//     .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
//     .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
//     .scan_interval = 0x50,
//     .scan_window = 0x30};

// // GAP callback
// static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     switch (event)
//     {

//     case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:

//         printf("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT\n");
//         if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS)
//         {
//             printf("Scan parameters set, start scanning for 10 seconds\n\n");
//             esp_ble_gap_start_scanning(10);
//         }
//         else
//             printf("Unable to set scan parameters, error code %d\n\n", param->scan_param_cmpl.status);
//         break;

//     case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:

//         printf("ESP_GAP_BLE_SCAN_START_COMPLETE_EVT\n");
//         if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
//         {
//             printf("Scan started\n\n");
//         }
//         else
//             printf("Unable to start scan process, error code %d\n\n", param->scan_start_cmpl.status);
//         break;

//     case ESP_GAP_BLE_SCAN_RESULT_EVT:

//         if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
//         {

//             // printf("ESP_GAP_BLE_SCAN_RESULT_EVT\n");
//             // printf("Device found: ADDR=");
//             // for (int i = 0; i < ESP_BD_ADDR_LEN; i++)
//             // {
//             //     printf("%02X", param->scan_rst.bda[i]);
//             //     if (i != ESP_BD_ADDR_LEN - 1)
//             //         printf(":");
//             // }

//             uint8_t *adv_name = NULL;
//             uint8_t adv_name_len = 0;
//             adv_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
//             if (adv_name)
//             {
//                 if (strncmp((char *)adv_name, "RC522-READER",12) != 0)
//                 {
//                     char *tmp_name = alloca(adv_name_len + 1);
//                     memcpy(tmp_name, adv_name, adv_name_len);
//                     tmp_name[adv_name_len] = '\0';
//                     device_seen((char *)tmp_name, param->scan_rst.bda, param->scan_rst.rssi);
//                 }
//                 else
//                 {

//                     uint8_t *msg = NULL;
//                     uint8_t msg_len = 0;
//                     msg = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE, &msg_len);

//                     uint64_t *sn = msg;

//                     rfid_seen(*sn, param->scan_rst.rssi);
//                 }
//             }

//             // printf("\n\n");
//         }
//         else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
//             esp_ble_gap_start_scanning(10);

//         break;

//     default:

//         printf("Event %d unhandled\n\n", event);
//         break;
//     }
// }

// void bt_start(struct BTConf *conf)
// {
//     heap_caps_print_heap_info(0);

//     printf("BT scan\n\n");

//     // set components to log only errors
//     // esp_log_level_set("*", ESP_LOG_ERROR);

//     // // initialize nvs
//     // ESP_ERROR_CHECK(nvs_flash_init());
//     // printf("- NVS init ok\n");

//     // release memory reserved for classic BT (not used)
//     ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
//     printf("- Memory for classic BT released\n");

//     // initialize the BT controller with the default config
//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     esp_bt_controller_init(&bt_cfg);
//     printf("- BT controller init ok\n");

//     // enable the BT controller in BLE mode
//     esp_bt_controller_enable(ESP_BT_MODE_BLE);
//     printf("- BT controller enabled in BLE mode\n");

//     // initialize Bluedroid library
//     esp_bluedroid_init();
//     esp_bluedroid_enable();
//     printf("- Bluedroid initialized and enabled\n");

//     // register GAP callback function
//     ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_gap_cb));
//     printf("- GAP callback registered\n\n");

//     // configure scan parameters
//     esp_ble_gap_set_scan_params(&ble_scan_params);

//     {
//         const esp_timer_create_args_t periodic_timer_args = {
//             .callback = &slow_timer_callback,
//             .name = "periodic",
//             .arg = conf};

//         /* Create timer for logging scanned devices. */
//         esp_timer_handle_t periodic_timer;
//         ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

//         /* Start periodic timer for 5 sec. */
//         ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 60000000)); // 60 sec
//     }
// }

#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"

static const char *tag = "NimBLE_BLE_PERIODIC_SYNC";
static int synced = 0;
static int periodic_sync_gap_event(struct ble_gap_event *event, void *arg);

void ble_store_config_init(void);

static void
periodic_sync_scan(void)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0)
    {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 0;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      periodic_sync_gap_event, NULL);
    if (rc != 0)
    {
        MODLOG_DFLT(ERROR, "Error initiating GAP discovery procedure; rc=%d\n",
                    rc);
    }
}

void print_periodic_sync_data(struct ble_gap_event *event)
{
    ESP_LOGE(TAG, "status : %d\nperiodic_sync_handle : %d\nsid : %d\n", event->periodic_sync.status, event->periodic_sync.sync_handle, event->periodic_sync.sid);
    ESP_LOGE(TAG, "adv addr : ");
    for (int i = 0; i < 6; i++)
    {
        ESP_LOGE(TAG, "%d ", event->periodic_sync.adv_addr.val[i]);
    }
    ESP_LOGE(TAG, "\nadv_phy : %s\n", event->periodic_sync.adv_phy == 1 ? "1m" : (event->periodic_sync.adv_phy == 2 ? "2m" : "coded"));
    ESP_LOGE(TAG, "per_adv_ival : %d\n", event->periodic_sync.per_adv_ival);
    ESP_LOGE(TAG, "adv_clk_accuracy : %d\n", event->periodic_sync.adv_clk_accuracy);
}
void print_periodic_adv_data(struct ble_gap_event *event)
{
    ESP_LOGE(TAG, "sync_handle : %d\n", event->periodic_report.sync_handle);
    ESP_LOGE(TAG, "tx_power : %d\n", event->periodic_report.tx_power);
    ESP_LOGE(TAG, "rssi : %d\n", event->periodic_report.rssi);
    ESP_LOGE(TAG, "data_status : %d\n", event->periodic_report.data_status);
    ESP_LOGE(TAG, "data_length : %d\n", event->periodic_report.data_length);
    ESP_LOGE(TAG, "data : ");
    for (int i = 0; i < event->periodic_report.data_length; i++)
    {
        ESP_LOGE(TAG, "%c", ((char *)event->periodic_report.data)[i]);
    }
    ESP_LOGE(TAG, "\n");
}
void print_periodic_sync_lost_data(struct ble_gap_event *event)
{
    ESP_LOGE(TAG, "sync_handle : %d\n", event->periodic_sync_lost.sync_handle);
    ESP_LOGE(TAG, "reason : %s\n", event->periodic_sync_lost.reason == 13 ? "timeout" : (event->periodic_sync_lost.reason == 14 ? "terminated locally" : "Unknown reason"));
}

/**
 * Utility function to log an array of bytes.
 */
void print_bytes(const uint8_t *bytes, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        ESP_LOGE(TAG, "%s0x%02x", i != 0 ? ":" : "", bytes[i]);
    }
}

void print_mbuf(const struct os_mbuf *om)
{
    int colon, i;

    colon = 0;
    while (om != NULL)
    {
        if (colon)
        {
            MODLOG_DFLT(INFO, ":");
        }
        else
        {
            colon = 1;
        }
        for (i = 0; i < om->om_len; i++)
        {
            MODLOG_DFLT(INFO, "%s0x%02x", i != 0 ? ":" : "", om->om_data[i]);
        }
        om = SLIST_NEXT(om, om_next);
    }
}

char *
addr_str(const void *addr)
{
    static char buf[6 * 2 + 5 + 1];
    const uint8_t *u8p;

    u8p = addr;
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);

    return buf;
}

void print_uuid(const ble_uuid_t *uuid)
{
    char buf[BLE_UUID_STR_LEN];

    ESP_LOGE(TAG, "%s", ble_uuid_to_str(uuid, buf));
}

int periodic_sync_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_EXT_DISC:
        /* An advertisment report was received during GAP discovery. */
        struct ble_gap_ext_disc_desc *disc = ((struct ble_gap_ext_disc_desc *)(&event->disc));

        char mac[6];
        memcpy(mac, disc->addr.val, 6);

        int rssi = disc->rssi;

        // for (int i = 0; i < disc->length_data; i++)
        // {
        //     printf("%c", disc->data[i]);
        // }
        // printf("\n");

        /*
        Packet.ADTypes = {
            0x01 : { name : "Flags", resolve: toStringArray },
            0x02 : { name : "Incomplete List of 16-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 2)},
            0x03 : { name : "Complete List of 16-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 2) },
            0x04 : { name : "Incomplete List of 32-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 4) },
            0x05 : { name : "Complete List of 32-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 4) },
            0x06 : { name : "Incomplete List of 128-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 16) },
            0x07 : { name : "Complete List of 128-bit Service Class UUIDs", resolve: toOctetStringArray.bind(null, 16) },
            0x08 : { name : "Shortened Local Name", resolve: toString },
            0x09 : { name : "Complete Local Name", resolve: toString },
            0x0A : { name : "Tx Power Level", resolve: toSignedInt },
            0x0D : { name : "Class of Device", resolve: toOctetString.bind(null, 3) },
            0x0E : { name : "Simple Pairing Hash C", resolve: toOctetString.bind(null, 16) },
            0x0F : { name : "Simple Pairing Randomizer R", resolve: toOctetString.bind(null, 16) },
            0x10 : { name : "Device ID", resolve: toOctetString.bind(null, 16) },
            // 0x10 : { name : "Security Manager TK Value", resolve: null }
            0x11 : { name : "Security Manager Out of Band Flags", resolve : toOctetString.bind(null, 16) },
            0x12 : { name : "Slave Connection Interval Range", resolve : toOctetStringArray.bind(null, 2) },
            0x14 : { name : "List of 16-bit Service Solicitation UUIDs", resolve : toOctetStringArray.bind(null, 2) },
            0x1F : { name : "List of 32-bit Service Solicitation UUIDs", resolve : toOctetStringArray.bind(null, 4) },
            0x15 : { name : "List of 128-bit Service Solicitation UUIDs", resolve : toOctetStringArray.bind(null, 8) },
            0x16 : { name : "Service Data", resolve : toOctetStringArray.bind(null, 1) },
            0x17 : { name : "Public Target Address", resolve : toOctetStringArray.bind(null, 6) },
            0x18 : { name : "Random Target Address", resolve : toOctetStringArray.bind(null, 6) },
            0x19 : { name : "Appearance" , resolve : null },
            0x1A : { name : "Advertising Interval" , resolve : toOctetStringArray.bind(null, 2)  },
            0x1B : { name : "LE Bluetooth Device Address", resolve : toOctetStringArray.bind(null, 6) },
            0x1C : { name : "LE Role", resolve : null },
            0x1D : { name : "Simple Pairing Hash C-256", resolve : toOctetStringArray.bind(null, 16) },
            0x1E : { name : "Simple Pairing Randomizer R-256", resolve : toOctetStringArray.bind(null, 16) },
            0x20 : { name : "Service Data - 32-bit UUID", resolve : toOctetStringArray.bind(null, 4) },
            0x21 : { name : "Service Data - 128-bit UUID", resolve : toOctetStringArray.bind(null, 16) },
            0x3D : { name : "3D Information Data", resolve : null },
            0xFF : { name : "Manufacturer Specific Data", resolve : null },
        }
        */

        int i = 0;
        char name[32];
        bool name_valid = false;
        char manufacturer[32];
        bool manufacturer_valid = false;
        while (i < disc->length_data)
        {
            int len = disc->data[i];
            int type = disc->data[i + 1];
            printf("Type: %#x, Len: %d, Data: [", type, len);
            for(int j = 0; j < len; j++)
            {
                printf("%c", disc->data[i+2+j]);
            }
            printf("]\n");

            if (type == 0xFF)
            {
                memcpy(manufacturer, &disc->data[i + 2], len);
                manufacturer_valid = true;
            }
            if (type == 0x09)
            {
                memcpy(name, &disc->data[i + 2], len);
                name_valid = true;
            }

            i += len + 2;
        }

        if (name_valid)
        {
            if (strncmp(name, "RC522-READER", 12) == 0)
            {
                if (manufacturer_valid)
                {
                    rfid_seen(*(uint64_t *)manufacturer, rssi);
                }
            }
            else
            {
                device_seen(name, (uint8_t *)mac, rssi);
            }
        }
        return 0;
    case BLE_GAP_EVENT_PERIODIC_REPORT:

        MODLOG_DFLT(INFO, "Periodic adv report event: \n");
        print_periodic_adv_data(event);
        return 0;
    case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:

        MODLOG_DFLT(INFO, "Periodic sync lost\n");
        print_periodic_sync_lost_data(event);
        synced = 0;
        return 0;
    case BLE_GAP_EVENT_PERIODIC_SYNC:

        MODLOG_DFLT(INFO, "Periodic sync event : \n");
        print_periodic_sync_data(event);
        return 0;
    default:
        return 0;
    }
}

static void
periodic_sync_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
periodic_sync_on_sync(void)
{
    int rc;
    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Begin scanning for a peripheral to connect to. */
    periodic_sync_scan();
}

void periodic_sync_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

void bt_start(struct BTConf *conf)
{

    int rc;
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        MODLOG_DFLT(ERROR, "Failed to init nimble %d \n", ret);
        return;
    }

    /* Configure the host. */
    ble_hs_cfg.reset_cb = periodic_sync_on_reset;
    ble_hs_cfg.sync_cb = periodic_sync_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // /* Initialize data structures to track connected peers. */
    // rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    // assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("nimble_periodic_sync");
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();
    nimble_port_freertos_init(periodic_sync_host_task);

    {
        const esp_timer_create_args_t periodic_timer_args = {
            .callback = &slow_timer_callback,
            .name = "periodic",
            .arg = conf};

        /* Create timer for logging scanned devices. */
        esp_timer_handle_t periodic_timer;
        ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

        /* Start periodic timer for 5 sec. */
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 60000000)); // 60 sec
    }
}