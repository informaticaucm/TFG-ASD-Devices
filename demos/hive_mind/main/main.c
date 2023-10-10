#include <stdio.h>
#include "esp_http_client.h"
#include "connect_wifi.h"
#include "esp_mac.h"

const char *TAG = "Main";

char http_response[100];
int http_response_size = 0;

esp_err_t _http_event_handle_store_response(esp_http_client_event_t *evt)
{

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        printf("%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            for (int i = 0; i < evt->data_len; i++)
            {
                ESP_LOGI(TAG, "i:%d %c", i, ((char *)evt->data)[i]);
                http_response[i] = ((char *)evt->data)[i];
            }
            http_response_size = evt->data_len;
            http_response[http_response_size] = 0;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

char *snprint_mac(char *buf, int n, const unsigned char *mac)
{
    if (n < 17)
        return "not enought space to print mac";
    snprintf(buf, n, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

void hive_min

void app_main(void)
{
    // conect to wifi
    connect_wifi();

    // send ping

    unsigned char mac_base[6] = {0};
    esp_efuse_mac_get_default(mac_base);
    esp_read_mac(mac_base, ESP_MAC_WIFI_STA);
    char *url_buffer = alloca(100);

    {
        snprintf(url_buffer, 100, "http://192.168.1.54:3000/ping?mac=%s", snprint_mac(alloca(20), 20, mac_base));
    }

    esp_http_client_config_t request_conf = {
        .url = url_buffer,
        .event_handler = _http_event_handle_store_response,
    };
    esp_http_client_handle_t client = esp_http_client_init(&request_conf);
    ESP_ERROR_CHECK(esp_http_client_perform(client));

    ESP_LOGI(TAG, "Status = %d, content_length = %lld",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));

    esp_http_client_cleanup(client);
}
