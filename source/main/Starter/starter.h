#ifndef __STARTER_H__
#define __STARTER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

enum StarterState
{
    NoQRConfig,
    NoWifi,
    NoAuth,
    NoTB,
    NoBackend,
    Success,
};

enum StarterCommand
{
    QrInfo,
    AuthInfo,
    InvalidateConfig,
};

struct QRInfo
{
    char wifi_ssid[30];
    char wifi_psw[30];
    char thingsboard_url[URL_SIZE];
    char mqtt_broker_url[URL_SIZE];
    char device_name[50];
    int space_id;
    char provisioning_device_key[21];
    char provisioning_device_secret[21];
};

struct StarterMsg
{
    enum StarterCommand command;
    union
    {
        struct QRInfo qr;
        char access_token[21];

    } data;
};

struct ConnectionParameters
{
    bool qr_valid;
    struct QRInfo qr_info;
    bool access_token_valid;
    char access_token[21];
    bool totp_seed_valid;
    char totp_seed[17];
};

struct StarterConf
{
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_starter_queue;
    QueueHandle_t to_mqtt_queue;
};

void start_starter(struct StarterConf *conf);

#endif // __STARTER_H__