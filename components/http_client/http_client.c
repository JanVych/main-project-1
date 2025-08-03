#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_client.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <cJSON.h>

static const char *TAG = "http_client";

static esp_err_t _clientEventGetHandler(esp_http_client_event_handle_t evt)
{
    http_response_t* response = evt->user_data;
    switch (evt->event_id)
    { 
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG ,"HTTP_EVENT_ON_HEADER, %s : %s", evt->header_key, evt->header_value);
        if (response != NULL && !strcmp(evt->header_key, "Content-Type"))
        {
            response->content_type = malloc(strlen(evt->header_value) + 1);
            strcpy(response->content_type, evt->header_value);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG ,"HTTP_EVENT_ON_DATA, %.*s", evt->data_len, (char *)evt->data);
        if(response != NULL)
        {
            response->data = malloc(evt->data_len);
            response->data_len = evt->data_len;
            memcpy(response->data, evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG ,"HTTP_EVENT_ERROR");
        break;

    default:
        break;
    }
    return ESP_OK;
}

esp_err_t http_BuildUrl(bool ssl, const char *host, uint16_t port, const char *path, const char *query, char *url, int max_len) 
{
    if (host == NULL || url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int len = 0;

    if(ssl){
        len += snprintf(url + len, max_len - len, "https://");
    }
    else{
        len += snprintf(url + len, max_len - len, "http://");
    }

    len += snprintf(url + len, max_len - len, "%s", host);

    if (port != 0) {
        len += snprintf(url + len, max_len - len, ":%u", port);
    }

    if (path != NULL && strlen(path) > 0) {
        len += snprintf(url + len, max_len - len, "%s", path);
    } else {
        len += snprintf(url + len, max_len - len, "/");
    }

    if (query != NULL && strlen(query) > 0) {
        len += snprintf(url + len, max_len - len, "?%s", query);
    }
    if (len >= max_len) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

http_response_t* http_CreateResponse()
{
    http_response_t* response = (http_response_t*)calloc(1, sizeof(http_response_t));
    response->content_type = NULL;
    response->data = NULL;
    response->json = NULL;
    return response;
}

void http_CleanResponse(http_response_t* r)
{
    if (r != NULL)
    {
        free(r->content_type);
        r->content_type = NULL;

        free(r->data);
        r->data = NULL;

        if(cJSON_IsObject(r->json)){
            cJSON_Delete(r->json);
            r->json = NULL;
        }
    }
}

void http_DeleteResponse(http_response_t* r)
{
    if(r != NULL)
    {
        http_CleanResponse(r);
        free(r);
    }
}

static void _httpSend(char* url, esp_http_client_method_t method, http_response_t* response, char* data, char* data_type)
{
    esp_http_client_config_t config =
    {
        .url = url,
        .method = method,
        .event_handler = _clientEventGetHandler,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = false,
        .user_data = response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(data != NULL){
        esp_http_client_set_post_field(client, data, strlen(data));
    }
    if(data_type != NULL && method == HTTP_METHOD_POST){
        esp_http_client_set_header(client, "Content-Type", data_type);
    }
    else if(data_type != NULL && method == HTTP_METHOD_GET){
        esp_http_client_set_header(client, "Accept", data_type);
    }

    // esp_http_client_get_url(client, url, sizeof(url));
    // ESP_LOGI(TAG, "http method: %i, url: %s", method, url);

    esp_err_t result = esp_http_client_perform(client);
    if(response != NULL){
        if(result == ESP_OK){
            response->status = esp_http_client_get_status_code(client);
        }
        else{
            response->status = 0;
        }
    }
    esp_http_client_cleanup(client);
}

void http_Get(char* url, http_response_t* response)
{
    _httpSend(url, HTTP_METHOD_GET, response, NULL, NULL);
}

void http_Post(char* url, char* data, http_response_t* response)
{
    _httpSend(url, HTTP_METHOD_POST, response, data, NULL);
}

void http_GetJson(char* url, http_response_t* response)
{
    _httpSend(url, HTTP_METHOD_GET, response, NULL, "application/json");
    if(response != NULL){
        response->json = cJSON_ParseWithLength(response->data, response->data_len);
    }
}

/// @brief post json structure to url
/// @param url target address
/// @param json cJSON data structure
/// @param response if not null, convert response to json
void http_PostJson(char* url, cJSON* json, http_response_t* response)
{
    char* data = cJSON_Print(json);
    _httpSend(url, HTTP_METHOD_POST, response, data, "application/json");
    free(data);
    if(response != NULL){
        response->json = cJSON_ParseWithLength(response->data, response->data_len);
    }
}