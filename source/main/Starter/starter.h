#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

enum StarterState
{
    NoQRConfig,
    NoWifi,
    NoTBAuth,
    NoTB,
    NoBackendAuth,
    NoBackend,
    Success,
};

enum StarterCommand
{
    QrInfo,
    TBAuthInfo,
    BackendInfo,
    InvalidateConfig,
    NotifyMalfunction,
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
    char totp_form_base_url[URL_SIZE];
};

struct BackendInfo
{
    char totp_seed[17];
    int device_id;
    int totp_t0;
};

struct StarterMsg
{
    enum StarterCommand command;
    union
    {
        struct
        {
            bool invalidate_backend_auth;
            bool invalidate_thingsboard_auth;
            struct QRInfo qr_info;
        } qr;
        struct BackendInfo backend_info;
        char access_token[21];

    } data;
};

struct ConnectionParameters
{
    bool qr_info_valid;
    struct QRInfo qr_info;
    bool access_token_valid;
    char access_token[21];
    bool backend_info_valid;
    struct BackendInfo backend_info;
};

struct StarterConf
{
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_starter_queue;
    QueueHandle_t to_mqtt_queue;
};

void start_starter(struct StarterConf *conf);
