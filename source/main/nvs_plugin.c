#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
int inited = 0;

void init_nvs()
{
    if (inited)
        return;
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    inited = 1;
}

void j_nvs_set(char *key, void *buffer, int buffer_size)
{
    init_nvs();

    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    nvs_set_blob(my_handle, key, buffer, buffer_size);
    nvs_commit(my_handle);
}
int j_nvs_get(char *key, void *buffer, int buffer_size)
{
    init_nvs();

    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    size_t len = (size_t) buffer_size;
    return nvs_get_str(my_handle, key, buffer, &len);
}