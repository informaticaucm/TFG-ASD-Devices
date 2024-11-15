#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "./nvs_plugin.h"
#include "nvs.h"
int inited = 0;
#define STORAGE_NAMESPACE "storage"
#define TAG "nvs_plugin"

void init_nvs()
{
    if (inited)
        return;

    ESP_LOGE(TAG, "initing flash");
    esp_err_t err = nvs_flash_init();
    ESP_LOGE(TAG, "initing flash finished with error %s", esp_err_to_name(err));

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    inited = 1;
}

void j_nvs_reset(char *key)
{
    init_nvs();

    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_erase_key(my_handle, key));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}

void j_nvs_set(char *key, void *buffer, int buffer_size)
{
    init_nvs();

    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle));
    ESP_ERROR_CHECK(nvs_set_blob(my_handle, key, buffer, buffer_size));
    ESP_ERROR_CHECK(nvs_commit(my_handle));
    nvs_close(my_handle);
}
int j_nvs_get(char *key, void *buffer, int buffer_size)
{

    init_nvs();

    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle)); // needs to be readwrite to create namespace
    size_t len = (size_t)buffer_size;
    int err = nvs_get_blob(my_handle, key, buffer, &len);
    nvs_close(my_handle);

    return err;
}