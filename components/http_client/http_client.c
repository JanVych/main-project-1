#include <stdio.h>
#include "http_client.h"
#include "esp_http_client.h"
#include "esp_netif.h"

static const char *TAG = "http_client";

static esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;
    
    case HTTP_EVENT_ERROR:
        break;

    default:
        break;
    }
    return ESP_OK;
}

void httpGet()
{
    esp_http_client_config_t config_get =
    {
        .url = "http://worldclockapi.com/api/json/utc/now",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler
    };
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}