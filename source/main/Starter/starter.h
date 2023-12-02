#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../common.h"

enum StarterCommand
{
    QrInfo,
    ProvisioningInfo,
    UnvalidateConfig,
};

struct StarterMsg
{
    enum StarterCommand command;
    union
    {
        struct
        {
            char wifi_ssid[30];
            char wifi_psw[30];
            char thingsboard_url[URL_SIZE];
            char mqtt_broker_url[URL_SIZE];
            char device_name[50];
            char provisioning_device_key[21];
            char provisioning_device_secret[21];
        } qr;
        struct
        {
            char access_token[21];
        } provisioning;

    } data;
};

struct ConfigurationParameters
{
    char wifi_ssid[30];
    char wifi_psw[30];
    char thingsboard_url[URL_SIZE];
    char mqtt_broker_url[URL_SIZE];
    bool provisioning_done;
    char device_name[50];
    bool valid;
    union
    {
        struct
        {
            char provisioning_device_key[21];
            char provisioning_device_secret[21];
        } due;
        struct
        {
            char access_token[21];
        } done;
    } provisioning;
};

struct StarterConf
{
    QueueHandle_t to_screen_queue;
    QueueHandle_t to_starter_queue;
    QueueHandle_t to_mqtt_queue;
};

void start_starter(struct StarterConf *conf);