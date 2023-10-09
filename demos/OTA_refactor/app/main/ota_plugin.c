#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "../../common/nvs_plugin.h"
#include "../../common/ota_version_fetcher.h"
#include "esp_ota_ops.h"

char *TAG = "OTA_PLUGIN";

void backtofactory()
{
    esp_partition_iterator_t pi;    // Iterator for find
    const esp_partition_t *factory; // Factory partition
    esp_err_t err;

    pi = esp_partition_find(ESP_PARTITION_TYPE_APP,            // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_APP_FACTORY, // factory partition
                            "factory");
    if (pi == NULL) // Check result
    {
        ESP_LOGE(TAG, "Failed to find factory partition");
    }
    else
    {
        factory = esp_partition_get(pi);           // Get partition struct
        esp_partition_iterator_release(pi);        // Release the iterator
        err = esp_ota_set_boot_partition(factory); // Set partition for boot
        if (err != ESP_OK)                         // Check error
        {
            ESP_LOGE(TAG, "Failed to set boot partition");
        }
        else
        {
            esp_restart(); // Restart ESP
        }
    }
}

void test_if_ota_is_due()
{
    int current_running_version = get_ota_version();
    int current_posted_version = http_fetch_ota_version();

    ESP_LOGE(TAG, "version %d vs %d", current_running_version, current_posted_version);

    if (current_running_version != current_posted_version)
    {
        backtofactory();
    }
}
