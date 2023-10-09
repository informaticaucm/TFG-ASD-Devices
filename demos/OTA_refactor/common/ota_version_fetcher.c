#include "esp_http_client.h"
#include "esp_log.h"
#include "config.h"

char http_response[100];
int http_response_size = 0;

static const char *TAG = "ota_version_fetch";

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

int http_fetch_ota_version()
{
    esp_http_client_config_t config = {
        .url = VERSION_URL,
        .event_handler = _http_event_handle_store_response,
    };

    config.skip_cert_common_name_check = true;

    ESP_LOGI(TAG, "Attempting to download metadata from %s", config.url);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_ERROR_CHECK(esp_http_client_perform(client));

    ESP_LOGI(TAG, "Status = %d, content_length = %lld",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));

    ESP_LOGI(TAG, "%s -> %s", config.url, http_response);
    esp_http_client_cleanup(client);

    return atoi(http_response);
}