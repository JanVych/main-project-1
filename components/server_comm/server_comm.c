#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
//#include "esp_flash.h"
#include "nvs.h"

#include "server_comm.h"
#include "http_client.h"
#include "wifi.h"

static const char *TAG = "server_comm";

static char* sever_address = "http://192.168.0.103:45455/api/modules";
static char* sever_actions_address = "http://192.168.0.103:45455/api/actions?id=1";

static uint32_t comm_interval_sec = 60;
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

static void getDeviceInfo(cJSON* json)
{
    nvs_handle_t handle;
    nvs_open("storage", NVS_READONLY, &handle);
    int64_t id;
    size_t len;
    char* str;
    if (nvs_get_i64(handle, "ModuleId", &id) == ESP_OK){
        cJSON_AddNumberToObject(json, "ModuleId", id);
    }
    if (nvs_get_str(handle,"ModuleName", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "ModuleName", str, &len);
        cJSON_AddStringToObject(json, "ModuleName", str);
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
    cJSON_AddStringToObject(json, "AppVersion", app_version);
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
    str = wifiGetAvailableNetworks();
    if(str != NULL){
        cJSON_AddStringToObject(json, "WifiList", str);
        free(str);
    }
    nvs_close(handle);
}

static const char* get_chip_name() 
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

static void mainLoop()
{
    http_response_t *actions_response = httpCreateResponse();
    while(true)
    {
        getDeviceInfo(json_to_send);
        
        httpPostJson(sever_address, json_to_send, NULL);

        httpGetJson(sever_actions_address, actions_response);
        if (actions_response->status == 200)
        {
            cJSON *item = NULL;
            cJSON_ArrayForEach(item, actions_response->json){
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

static server_comm_action_t* creteAction(char* name, serverCommCallback callback)
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
    if (actions == NULL){
        actions = creteAction(name, callback);
    }
    else
    {
        server_comm_action_t *current_action = actions;
        while(current_action->next != NULL){
            current_action = current_action->next;
        }
        current_action->next = creteAction(name, callback);
    }
}

void commDeleteAction(char* name)
{
}

static void defaultCallback(char* str){
    ESP_LOGI(TAG ,"callback with arg: %s", str);
}

static void init()
{ 
    commAddAction("default", defaultCallback);

    json_to_send = cJSON_CreateObject();

    const esp_app_desc_t *desc = esp_app_get_description();
    chip_name = get_chip_name();
    app_version = desc->version;
    idf_version = desc->idf_ver;

    xTaskCreate(mainLoop, "server_comm", 4096, NULL, tskIDLE_PRIORITY, &server_comm_task);
    is_initialized = true;
}

void commStart()
{
    if(is_initialized){
        vTaskResume(server_comm_task);
    }
    else {
        init();
    }
}

void commStop()
{
    vTaskSuspend(server_comm_task);
}


