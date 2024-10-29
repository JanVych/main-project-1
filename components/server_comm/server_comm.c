#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "esp_random.h"

#include "server_comm.h"
#include "http_client.h"
#include "wifi.h"

static const char *TAG = "server_comm";

static char* server_address = "http://192.168.0.107:45455/";
static char* server_host = "192.168.0.107";
static uint32_t server_port = 45455;

static uint32_t comm_interval_sec = 120;
static TaskHandle_t server_comm_task;
static bool is_initialized = false;
static cJSON* json_to_send;

static const char* chip_name;
static const char* app_version;
static const char* idf_version;

typedef void (*serverCommCallback)(char*);
typedef struct server_comm_action_t{
    
    char *name;
    serverCommCallback callback;
    struct server_comm_action_t *next;
}server_comm_action_t;

static server_comm_action_t *actions = NULL;
//bool lock_actions = false;


// TODO delete - use - esp_http_client_config_t
static void buildUrl(char *url, int16_t url_size, char *address, char *path)
{
    url[0] = 0;
    strncat(url, address, url_size - strlen(url) - 1);
    strncat(url, path, url_size - strlen(url) - 1);
}
// // TODO delete - use - esp_http_client_config_t
// static void builUrlWithQueryId(char *url, int16_t url_size, char *address, char *path, int64_t id)
// {
//     char id_buffer[21];
//     snprintf(id_buffer, sizeof(id_buffer), "%lld", id);
//     buildUrl(url, url_size, address, path);
//     strncat(url, "?id=", url_size - strlen(url) - 1);
//     strncat(url, id_buffer, url_size - strlen(url) - 1);
// }

static void getDeviceInfo(cJSON* json)
{
    nvs_handle_t handle;
    nvs_open("storage", NVS_READONLY, &handle);
    int64_t id;
    size_t len;
    char* str;
    if (nvs_get_i64(handle, "ModuleId", &id) == ESP_OK){
        cJSON_AddNumberToObject(json, "Id", id);
    }
    if (nvs_get_str(handle,"ModuleName", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "ModuleName", str, &len);
        cJSON_AddStringToObject(json, "Name", str);
        free(str);
    }
    if (nvs_get_str(handle,"ModuleKey", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "ModuleKey", str, &len);
        cJSON_AddStringToObject(json, "Key", str);
        free(str);
    }
    if (nvs_get_str(handle,"ProgramName", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "ProgramName", str, &len);
        cJSON_AddStringToObject(json, "ProgramName", str);
        free(str);
    }
    if (nvs_get_str(handle,"ProgramVersion", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "ProgramVersion", str, &len);
        cJSON_AddStringToObject(json, "ProgramVersion", str);
        free(str);
    }

    cJSON_AddStringToObject(json, "Chip", chip_name);
    cJSON_AddStringToObject(json, "FirmwareVersion", app_version);
    cJSON_AddStringToObject(json, "IDFVersion", idf_version);
    cJSON_AddNumberToObject(json, "FreeHeap", (double)esp_get_free_heap_size());

    //esp_flash_get_size()
    // uint32_t flash_size;
    // if (esp_flash_get_physical_size(NULL, &flash_size) != ESP_OK) {
    //     ESP_LOGE(TAG, "Get flash size failed");
    //     cJSON_AddNullToObject(json_to_send, "FlashSize");
    // }
    // else {
    //     cJSON_AddNumberToObject(json_to_send, "FlashSize", flash_size);
    // }
    str = wifiGetSTASsid();
    if(str != NULL){
        cJSON_AddStringToObject(json, "WifiCurrent", str);
        free(str);
    }
    // str = wifiGetAvailableNetworks();
    // if(str != NULL){
    //     cJSON_AddStringToObject(json, "WifiInRange", str);
    //     free(str);
    // }
    // nvs_close(handle);
}

static const char* _get_chip_name() 
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    switch (chip_info.model) {
        case CHIP_ESP32: return "ESP32";
        case CHIP_ESP32S2: return "ESP32-S2";
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32C3: return "ESP32-C3";
        case CHIP_ESP32C2: return "ESP32-C2";
        case CHIP_ESP32C6: return "ESP32-C6";
        case CHIP_ESP32H2: return "ESP32-H2";
        case CHIP_ESP32P4: return "ESP32-P4";
        case CHIP_POSIX_LINUX: return "POSIX/Linux simulator";
        default: return "Unknown";
    }
}

static void _processActions(cJSON *json_actions)
{
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, json_actions){
        bool found = false;
        if (cJSON_IsString(item)) {
            server_comm_action_t * current_action = actions;
            while (current_action != NULL) 
            {
                if(!strcmp(current_action->name, item->string))
                {
                    current_action->callback(item->valuestring);
                    found = true;
                }
                current_action = current_action->next;
            };
            if(!found){
                ESP_LOGI(TAG ,"callback not registrated,name: %s, arg: %s",item->string ,item->valuestring);
            }
        }
    }
}

// TODO add security to headers - https://github.com/espressif/esp-idf/issues/3097
static void _mainLoop()
{
    http_response_t *actions_response = httpCreateResponse();
    char str[12];
    while(true)
    {
        while (!wifiIsSTAConnected())
        {
            //ESP_LOGI(TAG ,"Waiting to wifi connection");
            vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);
        }
        int64_t id;
        char server_url[100];
        nvs_handle_t handle;
        nvs_open("storage", NVS_READONLY, &handle);
        if (nvs_get_i64(handle, "ModuleId", &id)){
            id = 0;
        }
        nvs_close(handle);

        buildUrl(server_url, 100, server_address, "api/modules");
        getDeviceInfo(json_to_send);

        sprintf(str, "%lu", esp_random());
        cJSON_AddStringToObject(json_to_send, "test", str);
        sprintf(str, "%lu", esp_random());
        cJSON_AddStringToObject(json_to_send, "test2", str);

        httpPostJson(server_url, json_to_send, actions_response);

        //builUrlWithQueryId(server_url, 100, server_address, "/api/actions", id);
        //httpGetJson(server_url, actions_response);

        if (actions_response->status == 200){
            ESP_LOGI(TAG ,"Response OK, processing actions...");
            _processActions(actions_response->json);
        }
        else{
            ESP_LOGI(TAG ,"Error, get actions, status code: %ld", actions_response->status);
        }

        httpCleanResponse(actions_response);
        cJSON_Delete(json_to_send);
        json_to_send = cJSON_CreateObject();

        vTaskDelay(comm_interval_sec * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void _performOTA(char* programName)
{
    if(wifiIsSTAConnected())
    {
        char query[50];
        snprintf(query, sizeof(query), "program=%s", programName);

        ESP_LOGI(TAG, "Starting OTA");
        esp_http_client_config_t config = 
        {
            .host = server_host,
            .port = server_port,
            // .url = "http://192.168.0.107:45455/api/firmware?program=projekt-1",
            .path = "/api/firmware",
            .query = query,
            .keep_alive_enable = true,
            .cert_pem = NULL,
        };

        esp_https_ota_config_t ota_config = {
            .http_config = &config,
        };

        //ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
        esp_err_t ret = esp_https_ota(&ota_config);
        if (ret == ESP_OK)
        {
            // nvs_handle_t handle;
            // nvs_open("storage", NVS_READWRITE, &handle);
            // nvs_set_str(handle, "ProgramName", programName);
            // nvs_commit(handle);
            // nvs_close(handle);
            ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
            esp_restart();
        } 
        else 
        {
            ESP_LOGE(TAG, "Firmware upgrade failed");
            return;
        }
    }
    ESP_LOGE(TAG, "Firmware upgrade failed, no internet connection");
}

static void _setModuleName(char *newName)
{
    ESP_LOGI(TAG ,"set module name to: %s",newName);
    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ModuleName", newName);
    nvs_commit(handle);
    nvs_close(handle);

}

static void _setModuleKey(char *newKey)
{
    ESP_LOGI(TAG ,"set module key to: %s",newKey);
    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ModuleKey", newKey);
    nvs_commit(handle);
    nvs_close(handle);

}

static void _setModuleId(char *newId)
{
    ESP_LOGI(TAG ,"set module id to: %s",newId);
    nvs_handle_t handle;
    uint64_t id;
    char *endptr;
    id = strtoll(newId, &endptr, 10);
    if(*endptr == '\0')
    {
        nvs_open("storage", NVS_READWRITE, &handle);
        nvs_set_i64(handle, "ModuleId", id);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static void defaultCallback(char* str){
    ESP_LOGI(TAG ,"callback with arg: %s", str);
}

static server_comm_action_t* _creteAction(char* name, serverCommCallback callback)
{
    server_comm_action_t *action = (server_comm_action_t*)calloc(1, sizeof(server_comm_action_t));
    uint16_t len = strlen(name) + 1;
    action->name = (char*)malloc(len);
    strlcpy(action->name, name, len);
    action->callback = callback;
    action->next = NULL;
    return action;
}

void commAddAction(char* name, serverCommCallback callback)
{
    server_comm_action_t *current_action = actions;
    server_comm_action_t *prew_action = NULL;

    if (actions == NULL){
        actions = _creteAction(name, callback);
    }
    else
    {
        while(current_action != NULL)
        {
            if(strcmp(current_action->name, name) == 0 && current_action->callback == callback)
            {
                ESP_LOGI(TAG ,"action name: %s, with callback: %p, already exist", name, callback);
                return;
            }
            prew_action = current_action;
            current_action = current_action->next;
        }
        prew_action->next = _creteAction(name, callback);
    }
}


void commDeleteAction(char* name, serverCommCallback callback)
{
    server_comm_action_t *prew_action = NULL;
    server_comm_action_t *current_action = actions;
    
    while(current_action != NULL)
    {
        if(strcmp(current_action->name, name) == 0 && current_action->callback == callback)
        {
            if(prew_action == NULL){
                actions = NULL;
            }
            else{
                prew_action->next = current_action->next;
            }
            free(current_action->name);
            free(current_action);
            return;
        }
        prew_action = current_action;
        current_action = current_action->next;
    }
}

void commAddMessage(char* key, char* value)
{
    cJSON *existing_item = cJSON_GetObjectItem(json_to_send, key);
    if (existing_item == NULL)
    {
         cJSON_AddStringToObject(json_to_send, key, value);
    }
}

static void _init()
{ 
    commAddAction("Default", defaultCallback);
    commAddAction("PerformOTA", _performOTA);
    commAddAction("SetModuleId", _setModuleId);
    commAddAction("SetModuleName", _setModuleName);
    commAddAction("SetModuleKey", _setModuleKey);

    json_to_send = cJSON_CreateObject();

    const esp_app_desc_t *desc = esp_app_get_description();
    chip_name = _get_chip_name();
    app_version = desc->version;
    idf_version = desc->idf_ver;

    xTaskCreate(_mainLoop, "server_comm", 4096, NULL, tskIDLE_PRIORITY, &server_comm_task);
    is_initialized = true;
}

void commStart()
{
    if(is_initialized){
        vTaskResume(server_comm_task);
    }
    else {
        _init();
    }
}

void commStop()
{
    vTaskSuspend(server_comm_task);
}


