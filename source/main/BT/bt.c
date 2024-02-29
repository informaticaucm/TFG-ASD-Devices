// /*
//  * BLE Combined Advertising and Scanning Example.
//  *
//  * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
//  *
//  * SPDX-License-Identifier: Unlicense OR CC0-1.0
//  */
// Bluetooth specific includes
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
#include <esp_log.h>
#include "bt.h"

static const char *TAG = "BT_BACKEND";

// typedef struct
// {
//     char scan_local_name[32];
//     uint8_t name_len;
// } ble_scan_local_name_t;

// typedef struct
// {
//     uint8_t *q_data;
//     uint16_t q_data_len;
// } host_rcv_data_t;

// static uint8_t hci_cmd_buf[128];

// static QueueHandle_t adv_queue;

// /*
//  * @brief: BT controller callback function, used to notify the upper layer that
//  *         controller is ready to receive command
//  */
// static void controller_rcv_pkt_ready(void)
// {
//     ESP_LOGI(TAG, "controller rcv pkt ready");
// }

// /*
//  * @brief: BT controller callback function to transfer data packet to
//  *         the host
//  */
// static int host_rcv_pkt(uint8_t *data, uint16_t len)
// {
//     host_rcv_data_t send_data;
//     uint8_t *data_pkt;
//     /* Check second byte for HCI event. If event opcode is 0x0e, the event is
//      * HCI Command Complete event. Sice we have recieved "0x0e" event, we can
//      * check for byte 4 for command opcode and byte 6 for it's return status. */
//     if (data[1] == 0x0e)
//     {
//         if (data[6] == 0)
//         {
//             ESP_LOGI(TAG, "Event opcode 0x%02x success.", data[4]);
//         }
//         else
//         {
//             ESP_LOGE(TAG, "Event opcode 0x%02x fail with reason: 0x%02x.", data[4], data[6]);
//             return ESP_FAIL;
//         }
//     }

//     data_pkt = (uint8_t *)malloc(sizeof(uint8_t) * len);
//     if (data_pkt == NULL)
//     {
//         ESP_LOGE(TAG, "Malloc data_pkt failed!");
//         return ESP_FAIL;
//     }
//     memcpy(data_pkt, data, len);
//     send_data.q_data = data_pkt;
//     send_data.q_data_len = len;
//     if (xQueueSend(adv_queue, (void *)&send_data, (TickType_t)0) != pdTRUE)
//     {
//         ESP_LOGD(TAG, "Failed to enqueue advertising report. Queue full.");
//         /* If data sent successfully, then free the pointer in `xQueueReceive'
//          * after processing it. Or else if enqueue in not successful, free it
//          * here. */
//         free(data_pkt);
//     }
//     return ESP_OK;
// }

// static esp_vhci_host_callback_t vhci_host_cb = {
//     controller_rcv_pkt_ready,
//     host_rcv_pkt};

// static void hci_cmd_send_reset(void)
// {
//     uint16_t sz = make_cmd_reset(hci_cmd_buf);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
// }

// static void hci_cmd_send_set_evt_mask(void)
// {
//     /* Set bit 61 in event mask to enable LE Meta events. */
//     uint8_t evt_mask[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20};
//     uint16_t sz = make_cmd_set_evt_mask(hci_cmd_buf, evt_mask);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
// }

// static void hci_cmd_send_ble_scan_params(void)
// {
//     /* Set scan type to 0x01 for active scanning and 0x00 for passive scanning. */
//     uint8_t scan_type = 0x01;

//     /* Scan window and Scan interval are set in terms of number of slots. Each slot is of 625 microseconds. */
//     uint16_t scan_interval = 0x50; /* 50 ms */
//     uint16_t scan_window = 0x30;   /* 30 ms */

//     uint8_t own_addr_type = 0x00; /* Public Device Address (default). */
//     uint8_t filter_policy = 0x00; /* Accept all packets excpet directed advertising packets (default). */
//     uint16_t sz = make_cmd_ble_set_scan_params(hci_cmd_buf, scan_type, scan_interval, scan_window, own_addr_type, filter_policy);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
// }

// static void hci_cmd_send_ble_scan_start(void)
// {
//     uint8_t scan_enable = 0x01;       /* Scanning enabled. */
//     uint8_t filter_duplicates = 0x00; /* Duplicate filtering disabled. */
//     uint16_t sz = make_cmd_ble_set_scan_enable(hci_cmd_buf, scan_enable, filter_duplicates);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
//     ESP_LOGI(TAG, "BLE Scanning started..");
// }

// static void hci_cmd_send_ble_adv_start(void)
// {
//     uint16_t sz = make_cmd_ble_set_adv_enable(hci_cmd_buf, 1);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
//     ESP_LOGI(TAG, "BLE Advertising started..");
// }

// static void hci_cmd_send_ble_set_adv_param(void)
// {
//     /* Minimum and maximum Advertising interval are set in terms of slots. Each slot is of 625 microseconds. */
//     uint16_t adv_intv_min = 0x100;
//     uint16_t adv_intv_max = 0x100;

//     /* Connectable undirected advertising (ADV_IND). */
//     uint8_t adv_type = 0;

//     /* Own address is public address. */
//     uint8_t own_addr_type = 0;

//     /* Public Device Address */
//     uint8_t peer_addr_type = 0;
//     uint8_t peer_addr[6] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85};

//     /* Channel 37, 38 and 39 for advertising. */
//     uint8_t adv_chn_map = 0x07;

//     /* Process scan and connection requests from all devices (i.e., the White List is not in use). */
//     uint8_t adv_filter_policy = 0;

//     uint16_t sz = make_cmd_ble_set_adv_param(hci_cmd_buf,
//                                              adv_intv_min,
//                                              adv_intv_max,
//                                              adv_type,
//                                              own_addr_type,
//                                              peer_addr_type,
//                                              peer_addr,
//                                              adv_chn_map,
//                                              adv_filter_policy);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
// }

// static void hci_cmd_send_ble_set_adv_data(void)
// {
//     char *adv_name = "ESP-BLE-1";
//     uint8_t name_len = (uint8_t)strlen(adv_name);
//     uint8_t adv_data[31] = {0x02, 0x01, 0x06, 0x0, 0x09};
//     uint8_t adv_data_len;

//     adv_data[3] = name_len + 1;
//     for (int i = 0; i < name_len; i++)
//     {
//         adv_data[5 + i] = (uint8_t)adv_name[i];
//     }
//     adv_data_len = 5 + name_len;

//     uint16_t sz = make_cmd_ble_set_adv_data(hci_cmd_buf, adv_data_len, (uint8_t *)adv_data);
//     esp_vhci_host_send_packet(hci_cmd_buf, sz);
//     ESP_LOGI(TAG, "Starting BLE advertising with name \"%s\"", adv_name);
// }

// static esp_err_t get_local_name(uint8_t *data_msg, uint8_t data_len, ble_scan_local_name_t *scanned_packet)
// {
//     uint8_t curr_ptr = 0, curr_len, curr_type;
//     while (curr_ptr < data_len)
//     {
//         curr_len = data_msg[curr_ptr++];
//         curr_type = data_msg[curr_ptr++];
//         if (curr_len == 0)
//         {
//             return ESP_FAIL;
//         }

//         /* Search for current data type and see if it contains name as data (0x08 or 0x09). */
//         if (curr_type == 0x08 || curr_type == 0x09)
//         {
//             for (uint8_t i = 0; i < curr_len - 1; i += 1)
//             {
//                 scanned_packet->scan_local_name[i] = data_msg[curr_ptr + i];
//             }
//             scanned_packet->name_len = curr_len - 1;
//             return ESP_OK;
//         }
//         else
//         {
//             /* Search for next data. Current length includes 1 octate for AD Type (2nd octate). */
//             curr_ptr += curr_len - 1;
//         }
//     }
//     return ESP_FAIL;
// }

// void hci_evt_process(void *arg)
// {

//     struct BTConf *conf = (struct BTConf *)arg;

//     host_rcv_data_t *rcv_data = (host_rcv_data_t *)malloc(sizeof(host_rcv_data_t));
//     if (rcv_data == NULL)
//     {
//         ESP_LOGE(TAG, "Malloc rcv_data failed!");
//         return;
//     }
//     esp_err_t ret;

//     while (1)
//     {
//         uint8_t sub_event, num_responses, total_data_len, data_msg_ptr, hci_event_opcode;
//         uint8_t *queue_data = NULL, *event_type = NULL, *addr_type = NULL, *addr = NULL, *data_len = NULL, *data_msg = NULL;
//         short int *rssi = NULL;
//         uint16_t data_ptr;
//         ble_scan_local_name_t *scanned_name = NULL;
//         total_data_len = 0;
//         data_msg_ptr = 0;
//         if (xQueueReceive(adv_queue, rcv_data, portMAX_DELAY) != pdPASS)
//         {
//             ESP_LOGE(TAG, "Queue receive error");
//         }
//         else
//         {
//             /* `data_ptr' keeps track of current position in the received data. */
//             data_ptr = 0;
//             queue_data = rcv_data->q_data;

//             /* Parsing `data' and copying in various fields. */
//             hci_event_opcode = queue_data[++data_ptr];
//             if (hci_event_opcode == LE_META_EVENTS)
//             {
//                 /* Set `data_ptr' to 4th entry, which will point to sub event. */
//                 data_ptr += 2;
//                 sub_event = queue_data[data_ptr++];
//                 /* Check if sub event is LE advertising report event. */
//                 if (sub_event == HCI_LE_ADV_REPORT)
//                 {

//                     /* Get number of advertising reports. */
//                     num_responses = queue_data[data_ptr++];
//                     event_type = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
//                     if (event_type == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc event_type failed!");
//                         goto reset;
//                     }
//                     for (uint8_t i = 0; i < num_responses; i += 1)
//                     {
//                         event_type[i] = queue_data[data_ptr++];
//                     }

//                     /* Get advertising type for every report. */
//                     addr_type = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
//                     if (addr_type == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc addr_type failed!");
//                         goto reset;
//                     }
//                     for (uint8_t i = 0; i < num_responses; i += 1)
//                     {
//                         addr_type[i] = queue_data[data_ptr++];
//                     }

//                     /* Get BD address in every advetising report and store in
//                      * single array of length `6 * num_responses' as each address
//                      * will take 6 spaces. */
//                     addr = (uint8_t *)malloc(sizeof(uint8_t) * 6 * num_responses);
//                     if (addr == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc addr failed!");
//                         goto reset;
//                     }
//                     for (int i = 0; i < num_responses; i += 1)
//                     {
//                         for (int j = 0; j < 6; j += 1)
//                         {
//                             addr[(6 * i) + j] = queue_data[data_ptr++];
//                         }
//                     }

//                     /* Get length of data for each advertising report. */
//                     data_len = (uint8_t *)malloc(sizeof(uint8_t) * num_responses);
//                     if (data_len == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc data_len failed!");
//                         goto reset;
//                     }
//                     for (uint8_t i = 0; i < num_responses; i += 1)
//                     {
//                         data_len[i] = queue_data[data_ptr];
//                         total_data_len += queue_data[data_ptr++];
//                     }

//                     if (total_data_len != 0)
//                     {
//                         /* Get all data packets. */
//                         data_msg = (uint8_t *)malloc(sizeof(uint8_t) * total_data_len);
//                         if (data_msg == NULL)
//                         {
//                             ESP_LOGE(TAG, "Malloc data_msg failed!");
//                             goto reset;
//                         }
//                         for (uint8_t i = 0; i < num_responses; i += 1)
//                         {
//                             for (uint8_t j = 0; j < data_len[i]; j += 1)
//                             {
//                                 data_msg[data_msg_ptr++] = queue_data[data_ptr++];
//                             }
//                         }
//                     }

//                     /* Counts of advertisements done. This count is set in advertising data every time before advertising. */
//                     rssi = (short int *)malloc(sizeof(short int) * num_responses);
//                     if (data_len == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc rssi failed!");
//                         goto reset;
//                     }
//                     for (uint8_t i = 0; i < num_responses; i += 1)
//                     {
//                         rssi[i] = -(0xFF - queue_data[data_ptr++]);
//                     }

//                     /* Extracting advertiser's name. */
//                     data_msg_ptr = 0;
//                     scanned_name = (ble_scan_local_name_t *)malloc(num_responses * sizeof(ble_scan_local_name_t));
//                     if (data_len == NULL)
//                     {
//                         ESP_LOGE(TAG, "Malloc scanned_name failed!");
//                         goto reset;
//                     }
//                     for (uint8_t i = 0; i < num_responses; i += 1)
//                     {
//                         ret = get_local_name(&data_msg[data_msg_ptr], data_len[i], scanned_name);

//                         /* Print the data if adv report has a valid name. */
//                         if (ret == ESP_OK)
//                         {
//                             // printf("******** Response %d/%d ********\n", i + 1, num_responses);
//                             // printf("Event type: %02x\nAddress type: %02x\nAddress: ", event_type[i], addr_type[i]);
//                             // for (int j = 5; j >= 0; j -= 1)
//                             // {
//                             //     printf("%02x", addr[(6 * i) + j]);
//                             //     if (j > 0)
//                             //     {
//                             //         printf(":");
//                             //     }
//                             // }

//                             // printf("\nData length: %d", data_len[i]);
//                             // data_msg_ptr += data_len[i];
//                             // printf("\nAdvertisement Name: ");
//                             // for (int k = 0; k < scanned_name->name_len; k += 1)
//                             // {
//                             //     printf("%c", scanned_name->scan_local_name[k]);
//                             // }
//                             // printf("\nRSSI: %ddB\n", rssi[i]);

//                             char * tmp_name = alloca(scanned_name->name_len+1);
//                             memcpy(tmp_name, scanned_name->scan_local_name, scanned_name->name_len);
//                             tmp_name[scanned_name->name_len] = '\0';

//                             device_seen(tmp_name, addr + (6 * i), rssi[i]);
//                         }
//                     }

//                     /* Freeing all spaces allocated. */
//                 reset:
//                     free(scanned_name);
//                     free(rssi);
//                     free(data_msg);
//                     free(data_len);
//                     free(addr);
//                     free(addr_type);
//                     free(event_type);
//                 }
//             }
//             free(queue_data);
//         }
//         memset(rcv_data, 0, sizeof(host_rcv_data_t));
//     }
// }

// void bt_start(struct BTConf *conf)
// {
//     bool continue_commands = 1;
//     int cmd_cnt = 0;

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

//     /* Initialize NVS — it is used to store PHY calibration data */
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

//     ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
//     if (ret)
//     {
//         ESP_LOGI(TAG, "Bluetooth controller release classic bt memory failed: %s", esp_err_to_name(ret));
//         return;
//     }

//     if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
//     {
//         ESP_LOGI(TAG, "Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
//         return;
//     }

//     if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BLE)) != ESP_OK)
//     {
//         ESP_LOGI(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
//         return;
//     }

//     /* A queue for storing received HCI packets. */
//     adv_queue = xQueueCreate(15, sizeof(host_rcv_data_t));
//     if (adv_queue == NULL)
//     {
//         ESP_LOGE(TAG, "Queue creation failed");
//         return;
//     }

//     esp_vhci_host_register_callback(&vhci_host_cb);
//     while (continue_commands)
//     {
//         if (continue_commands && esp_vhci_host_check_send_available())
//         {
//             switch (cmd_cnt)
//             {
//             case 0:
//                 hci_cmd_send_reset();
//                 break;
//             case 1:
//                 hci_cmd_send_set_evt_mask();
//                 break;

//             /* Send advertising commands. */
//             case 2:
//                 hci_cmd_send_ble_set_adv_param();
//                 break;
//             case 3:
//                 hci_cmd_send_ble_set_adv_data();
//                 break;
//             case 4:
//                 hci_cmd_send_ble_adv_start();
//                 break;

//             /* Send scan commands. */
//             case 5:
//                 hci_cmd_send_ble_scan_params();
//                 break;
//             case 6:
//                 hci_cmd_send_ble_scan_start();
//                 break;
//             default:
//                 continue_commands = 0;
//                 break;
//             }
//             ++cmd_cnt;

//             // ESP_LOGI(TAG, "BLE Advertise, cmd_sent: %d", cmd_cnt);
//         }
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
//     jTaskCreate(&hci_evt_process, "hci_evt_process", 2048 * 16, conf, 0, MALLOC_CAP_SPIRAM);
// }

// static const char *btsig_gap_type(uint32_t gap_type) {
// 	switch (gap_type)
// 	{
// 		case 0x01: return "Flags";
// 		case 0x02: return "Incomplete List of 16-bit Service Class UUIDs";
// 		case 0x03: return "Complete List of 16-bit Service Class UUIDs";
// 		case 0x04: return "Incomplete List of 32-bit Service Class UUIDs";
// 		case 0x05: return "Complete List of 32-bit Service Class UUIDs";
// 		case 0x06: return "Incomplete List of 128-bit Service Class UUIDs";
// 		case 0x07: return "Complete List of 128-bit Service Class UUIDs";
// 		case 0x08: return "Shortened Local Name";
// 		case 0x09: return "Complete Local Name";
// 		case 0x0A: return "Tx Power Level";
// 		case 0x0D: return "Class of Device";
// 		case 0x0E: return "Simple Pairing Hash C/C-192";
// 		case 0x0F: return "Simple Pairing Randomizer R/R-192";
// 		case 0x10: return "Device ID/Security Manager TK Value";
// 		case 0x11: return "Security Manager Out of Band Flags";
// 		case 0x12: return "Slave Connection Interval Range";
// 		case 0x14: return "List of 16-bit Service Solicitation UUIDs";
// 		case 0x1F: return "List of 32-bit Service Solicitation UUIDs";
// 		case 0x15: return "List of 128-bit Service Solicitation UUIDs";
// 		case 0x16: return "Service Data - 16-bit UUID";
// 		case 0x20: return "Service Data - 32-bit UUID";
// 		case 0x21: return "Service Data - 128-bit UUID";
// 		case 0x22: return "LE Secure Connections Confirmation Value";
// 		case 0x23: return "LE Secure Connections Random Value";
// 		case 0x24: return "URI";
// 		case 0x25: return "Indoor Positioning";
// 		case 0x26: return "Transport Discovery Data";
// 		case 0x17: return "Public Target Address";
// 		case 0x18: return "Random Target Address";
// 		case 0x19: return "Appearance";
// 		case 0x1A: return "Advertising Interval";
// 		case 0x1B: return "LE Bluetooth Device Address";
// 		case 0x1C: return "LE Role";
// 		case 0x1D: return "Simple Pairing Hash C-256";
// 		case 0x1E: return "Simple Pairing Randomizer R-256";
// 		case 0x3D: return "3D Information Data";
// 		case 0xFF: return "Manufacturer Specific Data";

// 		default:
// 			return "Unknown type";
// 	}
// } // btsig_gap_type

// #define BT_BD_ADDR_HEX(addr) addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

// static void gap_callback_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     esp_ble_gap_cb_param_t *p = (esp_ble_gap_cb_param_t *)param;
//     esp_err_t status;

// 	// printf("BT payload rcvd -> type: 0x%.2x -> %s\n", *p->scan_rst.ble_adv, btsig_gap_type(*p->scan_rst.ble_adv));

//     switch (event)
//     {
//     case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
//     { // restart scan
//         status = esp_ble_gap_start_scanning(100);
//         if (status != ESP_OK)
//         {
//             ESP_LOGE(TAG, "esp_ble_gap_start_scanning: rc=%d", status);
//         }
//     }
//     break;

//     case ESP_GAP_BLE_SCAN_RESULT_EVT:
//     {
//         if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) // Inquiry complete, scan is done
//         {                                                          // restart scan
//             status = esp_ble_gap_start_scanning(100);
//             if (status != ESP_OK)
//             {
//                 printf("esp_ble_gap_start_scanning: rc=%d", status);
//             }
//             return;
//         }

//         if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) // Inquiry result for a peer device
//         {                                                         // evaluate sniffed packet

//             uint8_t len;
//             uint8_t *data = esp_ble_resolve_adv_data(p->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT, &len);
//             if (data)
//             {
//                 printf("Device address (bda): %02x:%02x:%02x:%02x:%02x:%02x\n", BT_BD_ADDR_HEX(p->scan_rst.bda));
//                 printf("RSSI                : %d\n", p->scan_rst.rssi);
//                 printf("Complete name       : %s\n", data);
//                 device_seen((char *)data, p->scan_rst.bda, p->scan_rst.rssi);
//             }

//             // Copy the parameter UNION
//             esp_ble_gap_cb_param_t scan_result = *p;

//             // printf("\n-------------------------\nMessage: \n");
//             // uint8_t adv_data_len = scan_result.scan_rst.adv_data_len;
//             // uint8_t *adv_data = scan_result.scan_rst.ble_adv;

//             // printf("Message length: %i\n", adv_data_len);
//             // printf("Message body:\n");

//             // for (int i = 0; i < adv_data_len; ++i)
//             // {
//             //     printf("%X", adv_data[i]);
//             // }
//             // printf("\n-------------------------\n");
//         }
//     }
//     break;

//     default:
//         break;
//     }
// } // gap_callback_handler

// esp_err_t register_ble_functionality(void)
// {
//     esp_err_t status;

//     ESP_LOGI(TAG, "Register GAP callback");

//     // This function is called to occur gap event, such as scan result.
//     // register the scan callback function to the gap module
//     status = esp_ble_gap_register_callback(gap_callback_handler);
//     if (status != ESP_OK)
//     {
//         ESP_LOGE(TAG, "esp_ble_gap_register_callback: rc=%d", status);
//         return ESP_FAIL;
//     }

//     static esp_ble_scan_params_t ble_scan_params =
//         {
//             .scan_type = BLE_SCAN_TYPE_PASSIVE,
//             .own_addr_type = BLE_ADDR_TYPE_RANDOM,
//             .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
//             .scan_interval = 10,
//             .scan_window = 10};

//     ESP_LOGI(TAG, "Set GAP scan parameters");

//     // This function is called to set scan parameters.
//     status = esp_ble_gap_set_scan_params(&ble_scan_params);
//     if (status != ESP_OK)
//     {
//         ESP_LOGE(TAG, "esp_ble_gap_set_scan_params: rc=%d", status);
//         return ESP_FAIL;
//     }

//     return ESP_OK;
// }

// void bt_start(struct BTConf *conf)
// {

//     // Initialize BT controller to allocate task and other resource.
//     ESP_LOGI(TAG, "Enabling Bluetooth Controller");
//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     if (esp_bt_controller_init(&bt_cfg) != ESP_OK)
//     {
//         ESP_LOGE(TAG, "Bluetooth controller initialize failed");
//     }

//     // Enable BT controller
//     ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
//     ESP_ERROR_CHECK(esp_bluedroid_init());
//     ESP_ERROR_CHECK(esp_bluedroid_enable());
//     ESP_ERROR_CHECK(register_ble_functionality());

//     ESP_LOGI("BT", "Started scanning for BLE devices");
// }

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

// scan parameters
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30};

// GAP callback
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:

        printf("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT\n");
        if (param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            printf("Scan parameters set, start scanning for 10 seconds\n\n");
            esp_ble_gap_start_scanning(10);
        }
        else
            printf("Unable to set scan parameters, error code %d\n\n", param->scan_param_cmpl.status);
        break;

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:

        printf("ESP_GAP_BLE_SCAN_START_COMPLETE_EVT\n");
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
        {
            printf("Scan started\n\n");
        }
        else
            printf("Unable to start scan process, error code %d\n\n", param->scan_start_cmpl.status);
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:

        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
        {

            // printf("ESP_GAP_BLE_SCAN_RESULT_EVT\n");
            // printf("Device found: ADDR=");
            // for (int i = 0; i < ESP_BD_ADDR_LEN; i++)
            // {
            //     printf("%02X", param->scan_rst.bda[i]);
            //     if (i != ESP_BD_ADDR_LEN - 1)
            //         printf(":");
            // }

            // try to read the complete name
            uint8_t *msg = NULL;
            uint8_t msg_len = 0;
            msg = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE, &msg_len);
            if (msg)
            {
                printf("\nMSG=");
                for (int i = 0; i < msg_len; i++)
                    printf("%c", msg[i]);
            }

            // // try to read the complete name
            // uint8_t *adv_name = NULL;
            // uint8_t adv_name_len = 0;
            // adv_name = esp_ble_resolve_adv_data(param->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            // if (adv_name)
            // {
            //     printf("\nFULL NAME=");
            //     for (int i = 0; i < adv_name_len; i++)
            //         printf("%c", adv_name[i]);
            //     device_seen((char *)adv_name, param->scan_rst.bda, param->scan_rst.rssi);
            // }

            // printf("\n\n");
        }
        else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
            esp_ble_gap_start_scanning(10);

        break;

    default:

        printf("Event %d unhandled\n\n", event);
        break;
    }
}

void bt_start(struct BTConf *conf)
{
    printf("BT scan\n\n");

    // set components to log only errors
    // esp_log_level_set("*", ESP_LOG_ERROR);

    // // initialize nvs
    // ESP_ERROR_CHECK(nvs_flash_init());
    // printf("- NVS init ok\n");

    // release memory reserved for classic BT (not used)
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    printf("- Memory for classic BT released\n");

    // initialize the BT controller with the default config
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    printf("- BT controller init ok\n");

    // enable the BT controller in BLE mode
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    printf("- BT controller enabled in BLE mode\n");

    // initialize Bluedroid library
    esp_bluedroid_init();
    esp_bluedroid_enable();
    printf("- Bluedroid initialized and enabled\n");

    // register GAP callback function
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_gap_cb));
    printf("- GAP callback registered\n\n");

    // configure scan parameters
    esp_ble_gap_set_scan_params(&ble_scan_params);
}