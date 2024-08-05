#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_client.h"
#include "esp_http_client.h"
#include "esp_netif.h"
//#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <cJSON.h>

static const char *TAG = "http_client";

static esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    http_response_t *response = evt->user_data;
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

http_response_t* httpCreateResponse()
{
    return (http_response_t*)calloc(1, sizeof(http_response_t));
}

void httpCleanResponse(http_response_t *r)
{
    if(r->content_type != NULL){
        free(r->content_type);
    }
    if(r->data != NULL){
        free(r->data);
    }
    if(cJSON_IsObject(r->json)){
        cJSON_Delete(r->json);
    }
}

void httpDeleteResponse(http_response_t *r)
{
    httpCleanResponse(r);
    free(r);
}

static void httpSend(char *url, esp_http_client_method_t method, http_response_t *response, char *data, char *data_type)
{
    esp_http_client_config_t config_get =
    {
        .url = url,
        .method = method,
        .event_handler = client_event_get_handler,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        //.crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    if(data != NULL){
        esp_http_client_set_post_field(client, data, strlen(data));
    }
    if(data_type != NULL && method == HTTP_METHOD_POST){
        esp_http_client_set_header(client, "Content-Type", data_type);
    }
    else if(data_type != NULL && method == HTTP_METHOD_GET){
        esp_http_client_set_header(client, "Accept", data_type);
    }
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

void httpGet(char *url, http_response_t *response)
{
    httpSend(url, HTTP_METHOD_GET, response, NULL, NULL);
}

void httpPost(char *url, char* data, http_response_t * response)
{
    httpSend(url, HTTP_METHOD_POST, response, data, NULL);
}

void httpGetJson(char *url, http_response_t *response)
{
    httpSend(url, HTTP_METHOD_GET, response, NULL, "application/json");
    response->json = cJSON_ParseWithLength(response->data, response->data_len);
}

void httpPostJson(char *url, cJSON* json, http_response_t *response)
{
    char *data = cJSON_Print(json);
    httpSend(url, HTTP_METHOD_POST, response, data, "application/json");
    free(data);
}