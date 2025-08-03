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
#include "esp_crt_bundle.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#include "server_comm.h"
#include "http_client.h"
#include "wifi.h"

#define LED_GPIO 27
static const char *TAG = "server_comm";
// static char* _serverHost = "krepenec.tplinkdns.com";
static char _serverAddress[120];
// static uint32_t _serverPort = 60000;
uint32_t _commIntervalSec = 60;

static TaskHandle_t _serverCommTask;
static bool _isInitialized = false;
static cJSON* _jsonToSend;
static bool _isHttpEnabled = false;

typedef enum 
{
    COMM_CALLBACK_STR,
    COMM_CALLBACK_I32,
    COMM_CALLBACK_FLOAT,
    COMM_CALLBACK_BOOL,
    COMM_CALLBACK_CJSON,
    COMM_CALLBACK_VOID
} callback_type_t;

typedef union message_callback_t
{
    char* (*returStr_f)();
    int32_t (*returnI32_f)();
    float (*returnFloat_f)();
    bool (*returnBool_f)();
    cJSON* (*returnCjson_f)();
} message_callback_t;

typedef union action_callback_u{
    void (*strCallback_f)(char*);
    void (*intCallback_f)(int32_t);
    void (*floatCallback_f)(float);
    void (*boolCallback_f)(bool);
    void (*jsonCallback_f)(cJSON*);
    void (*voidCallback_f)(void);
} action_callback_u;

typedef struct message_comm_struct_t
{
    char* key;
    callback_type_t type;
    message_callback_t callback;
    struct message_comm_struct_t* next;
} message_comm_struct_t;

//typedef void (*serverCommCallback)(char*);

typedef struct server_comm_action_t{
    char *key;
    callback_type_t type;
    action_callback_u callback;
    struct server_comm_action_t *next;
}server_comm_action_t;

static message_comm_struct_t* _messages = NULL;
static server_comm_action_t* _actions = NULL;

static const char* _GetChipName() 
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

static void _GetDeviceInfo(cJSON* json)
{
    nvs_handle_t handle;
    int64_t id;
    size_t len;
    char* str;

    nvs_open("config", NVS_READONLY, &handle);

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
    if (nvs_get_str(handle,"WifiPassword", NULL, &len) == ESP_OK){
        str = malloc(len);
        nvs_get_str(handle, "WifiPassword", str, &len);
        cJSON_AddStringToObject(json, "WifiPassword", str);
        free(str);
    }

    nvs_close(handle);
    const esp_app_desc_t *desc = esp_app_get_description();
    cJSON_AddStringToObject(json, "Chip", _GetChipName());
    cJSON_AddStringToObject(json, "FirmwareVersion", desc->version);
    cJSON_AddStringToObject(json, "IDFVersion", desc->idf_ver);
    cJSON_AddNumberToObject(json, "FreeHeap", (double)esp_get_free_heap_size());
    cJSON_AddNumberToObject(json, "CommInterval", _commIntervalSec);

    str = wifiGetSTASsid();
    if(str != NULL){
        cJSON_AddStringToObject(json, "WifiSsid", str);
        free(str);
    }
    // str = wifiGetAvailableNetworks();
    // if(str != NULL){
    //     cJSON_AddStringToObject(json, "WifiInRange", str);
    //     free(str);
    // }
}

static void _ProcessActions(cJSON *json_actions, server_comm_action_t* actions)
{
    cJSON *item = NULL;
    bool found;
    server_comm_action_t *current_action;
    cJSON_ArrayForEach(item, json_actions)
    {
        ESP_LOGI(TAG ,"action: %s",item->string);
        found = false;
        current_action = actions;
            while (current_action != NULL) 
            {
                if(!strcmp(current_action->key, item->string))
                {
                    found = true;
                    switch (current_action->type)
                    {
                        case COMM_CALLBACK_STR:
                            if(cJSON_IsString(item))
                            {
                                current_action->callback.strCallback_f(item->valuestring);
                            }
                            break;
                        case COMM_CALLBACK_I32:
                            if(cJSON_IsNumber(item))
                            {
                                current_action->callback.intCallback_f(item->valueint);
                            }
                            break;
                        case COMM_CALLBACK_FLOAT:
                            if(cJSON_IsNumber(item))
                            {
                                current_action->callback.floatCallback_f((float)item->valuedouble);
                            }
                            break;
                        case COMM_CALLBACK_BOOL:
                            if(cJSON_IsBool(item))
                            {
                                current_action->callback.boolCallback_f(cJSON_IsTrue(item));
                            }
                            break;
                        case COMM_CALLBACK_CJSON:
                            current_action->callback.jsonCallback_f(item);
                            break;
                        case COMM_CALLBACK_VOID:
                            current_action->callback.voidCallback_f();
                            break;
                    }
                }
                current_action = current_action->next;
            };
            if(!found){
                ESP_LOGI(TAG ,"callback not registrated,name: %s, arg: %s",item->string ,item->valuestring);
            }
    }
}

static void _LoadMessages(cJSON* json_messages, message_comm_struct_t* messages)
{
    message_comm_struct_t* current_message = messages;
    while(current_message != NULL)
    {
        switch (current_message->type)
        {
            case COMM_CALLBACK_STR:
                char* str = current_message->callback.returStr_f();
                cJSON_AddStringToObject(json_messages, current_message->key, str);
                break;
            case COMM_CALLBACK_I32:
                double int32 = current_message->callback.returnI32_f();
                cJSON_AddNumberToObject(json_messages, current_message->key, (double) int32);
                break;
            case COMM_CALLBACK_FLOAT:
                float floatValue = current_message->callback.returnFloat_f();
                cJSON_AddNumberToObject(json_messages, current_message->key, floatValue);
                break;
            case COMM_CALLBACK_BOOL:
                bool boolean = current_message->callback.returStr_f();
                cJSON_AddBoolToObject(json_messages, current_message->key, boolean);
                break;
            case COMM_CALLBACK_CJSON:
                cJSON* cJson = current_message->callback.returnCjson_f();
                cJSON_AddItemToObject(json_messages, current_message->key, cJson);
                break;
            case COMM_CALLBACK_VOID:
                cJSON_AddNullToObject(json_messages, current_message->key);
            
        }
        current_message = current_message->next;
    }
}

// TODO add security to headers - https://github.com/espressif/esp-idf/issues/3097
static void _MainLoop()
{
    http_response_t *actionsResponse = http_CreateResponse();
    char url[120];

    while(true)
    {
        while (!wifiIsSTAConnected())
        {
            //ESP_LOGI(TAG ,"Waiting to wifi connection");
            vTaskDelay(1 * 1000 / portTICK_PERIOD_MS);
        }

        _GetDeviceInfo(_jsonToSend);

        _LoadMessages(_jsonToSend, _messages);

        http_BuildUrl(!_isHttpEnabled, _serverAddress, 0, "/api/modules", NULL, url, sizeof(url));
        ESP_LOGI(TAG ,"Sending data to: %s", url);
        http_PostJson(url, _jsonToSend, actionsResponse);

        if (actionsResponse->status == 200){
            ESP_LOGI(TAG ,"Response OK, processing actions...");
            _ProcessActions(actionsResponse->json, _actions);
        }
        else{
            ESP_LOGI(TAG ,"POST to: %s, error, status code: %ld", url, actionsResponse->status);
        }

        http_CleanResponse(actionsResponse);
        cJSON_Delete(_jsonToSend);
        _jsonToSend = cJSON_CreateObject();

        vTaskDelay(_commIntervalSec * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void _PerformOTA(char* programName)
{
    if(wifiIsSTAConnected())
    {
        char url[120];
        char query[50];
        nvs_handle_t handle;
        char* oldProgramName;
        size_t len;

        snprintf(query, sizeof(query), "program=%s", programName);
        http_BuildUrl(!_isHttpEnabled, _serverAddress, 0, "/api/firmware", query, url, sizeof(url));
        ESP_LOGI(TAG, "Starting OTA, program: %s", programName);

        esp_http_client_config_t config = 
        {
            .url = url,
            .keep_alive_enable = true,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };                              
        esp_https_ota_config_t ota_config = {
            .http_config = &config,
        };

        ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
        esp_err_t ret = esp_https_ota(&ota_config);
        
        if (ret == ESP_OK)
        {
            nvs_open("config", NVS_READWRITE, &handle);
            if (nvs_get_str(handle,"ProgramName", NULL, &len) == ESP_OK)
            {
                oldProgramName = malloc(len);
                nvs_get_str(handle, "ProgramName", oldProgramName, &len);
                nvs_set_str(handle, "_ProgramName", oldProgramName);
                free(oldProgramName);
            }

            nvs_set_str(handle, "ProgramName", programName);
            nvs_commit(handle);
            nvs_close(handle);
            ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
            esp_restart();
        } 
        else 
        {
            ESP_LOGE(TAG, "Firmware upgrade failed");
            return;
        }
    }
    ESP_LOGE(TAG, "Firmware upgrade failed, no wifi connection");
}

static void _SetModuleName(char *newName)
{
    ESP_LOGI(TAG ,"set module name to: %s",newName);
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ModuleName", newName);
    nvs_commit(handle);
    nvs_close(handle);
}

static void _SetModuleKey(char *newKey)
{
    ESP_LOGI(TAG ,"set module key to: %s",newKey);
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ModuleKey", newKey);
    nvs_commit(handle);
    nvs_close(handle);
}

static void _SetServerAddress(char *newAddress)
{
    ESP_LOGI(TAG ,"set server address to: %s",newAddress);
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_str(handle, "ServerAddress", newAddress);
    nvs_commit(handle);
    nvs_close(handle);
    strlcpy(_serverAddress, newAddress, sizeof(_serverAddress));
}

static void _SetModuleId(char *newId)
{
    ESP_LOGI(TAG ,"set module id to: %s",newId);
    nvs_handle_t handle;
    uint64_t id;
    char *endptr;
    id = strtoll(newId, &endptr, 10);
    if(*endptr == '\0')
    {
        nvs_open("config", NVS_READWRITE, &handle);
        nvs_set_i64(handle, "ModuleId", id);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

static void _SetCommInterval(int32_t interval)
{
    ESP_LOGI(TAG ,"set module communication interval to: %ld", interval);
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_i32(handle, "CommInterval", interval);
    nvs_commit(handle);
    nvs_close(handle);
    _commIntervalSec = (uint32_t)interval;
}

static server_comm_action_t* _CreteAction(char* key)
{
    server_comm_action_t *action = (server_comm_action_t*)calloc(1, sizeof(server_comm_action_t));
    uint16_t len = strlen(key) + 1;
    action->key = (char*)malloc(len);
    strlcpy(action->key, key, len);
    action->next = NULL;
    return action;
}
// static server_comm_action_t* _CreteAction(char* key, serverCommCallback callback)
// {
//     server_comm_action_t *action = (server_comm_action_t*)calloc(1, sizeof(server_comm_action_t));
//     uint16_t len = strlen(key) + 1;
//     action->key = (char*)malloc(len);
//     strlcpy(action->key, key, len);
//     action->callback = callback;
//     action->next = NULL;
//     return action;
// }

static server_comm_action_t* _AddAction(char* name)
{
    server_comm_action_t *current_action = _actions;
    server_comm_action_t *prew_action = NULL;
    server_comm_action_t* action = _CreteAction(name);

    if (_actions == NULL){
        _actions = action;
    }
    else
    {
        while(current_action != NULL)
        {
            prew_action = current_action;
            current_action = current_action->next;
        }
        prew_action->next = action;
    }
    return action;
}

void comm_AddActionStr(char* name, void(*callback)(char*))
{
    server_comm_action_t* action = _AddAction(name);
    action->callback.strCallback_f = callback;
    action->type = COMM_CALLBACK_STR;
}

void comm_AddActionInt32(char* name, void(*callback)(int32_t))
{
    server_comm_action_t* action = _AddAction(name);
    action->callback.intCallback_f = callback;
    action->type = COMM_CALLBACK_I32;
}

void comm_AddActionBool(char* name, void(*callback)(bool))
{
    server_comm_action_t* action = _AddAction(name);
    action->callback.boolCallback_f = callback;
    action->type = COMM_CALLBACK_BOOL;
}

void comm_AddActionJson(char* name, void(*callback)(cJSON*))
{
    server_comm_action_t* action = _AddAction(name);
    action->callback.jsonCallback_f = callback;
    action->type = COMM_CALLBACK_CJSON;
}

void comm_AddActionVoid(char* name, void(*callback)(void))
{
    server_comm_action_t* action = _AddAction(name);
    action->callback.voidCallback_f = callback;
    action->type = COMM_CALLBACK_VOID;
}

// void comm_AddAction(char* name, serverCommCallback callback)
// {
//     server_comm_action_t *current_action = _actions;
//     server_comm_action_t *prew_action = NULL;

//     if (_actions == NULL){
//         _actions = _CreteAction(name, callback);
//     }
//     else
//     {
//         while(current_action != NULL)
//         {
//             if(strcmp(current_action->key, name) == 0 && current_action->callback == callback)
//             {
//                 ESP_LOGI(TAG ,"action name: %s, with callback: %p, already exist", name, callback);
//                 return;
//             }
//             prew_action = current_action;
//             current_action = current_action->next;
//         }
//         prew_action->next = _CreteAction(name, callback);
//     }
// }

// void comm_DeleteAction(char* name, serverCommCallback callback)
// {
//     server_comm_action_t *prew_action = NULL;
//     server_comm_action_t *current_action = _actions;
    
//     while(current_action != NULL)
//     {
//         if(strcmp(current_action->key, name) == 0 && current_action->callback == callback)
//         {
//             if(prew_action == NULL){
//                 _actions = NULL;
//             }
//             else{
//                 prew_action->next = current_action->next;
//             }
//             free(current_action->key);
//             free(current_action);
//             return;
//         }
//         prew_action = current_action;
//         current_action = current_action->next;
//     }
// }

void comm_PushMessage(char* key, char* value)
{
    cJSON *existing_item = cJSON_GetObjectItem(_jsonToSend, key);
    if (existing_item == NULL)
    {
         cJSON_AddStringToObject(_jsonToSend, key, value);
    }
    else
    {
        ESP_LOGI(TAG, "Message with key: %s already exists, updating value", key);
        cJSON_SetValuestring(existing_item, value);
    }
}

static message_comm_struct_t* _CreateMessage(char* key)
{
    message_comm_struct_t* message = (message_comm_struct_t*)calloc(1, sizeof(message_comm_struct_t));
    uint16_t len = strlen(key) + 1;
    message->key = (char*)malloc(len);
    strlcpy(message->key, key, len);
    message->next = NULL;
    return message;
}

static message_comm_struct_t* _AddMessage(char* key)
{
    if (_messages == NULL)
    {
        _messages = _CreateMessage(key);
        return _messages;
    }
    else
    {
        message_comm_struct_t* current_message = _messages;
        message_comm_struct_t* prew_message = NULL;
        while(current_message != NULL)
        {
            if(!strcmp(current_message->key, key))
            {
                ESP_LOGI(TAG ,"message with: key %s, already exist", key);
                return NULL;
            }
            prew_message = current_message;
            current_message = current_message->next;
        }
        current_message = _CreateMessage(key);
        prew_message->next = current_message;
        return current_message;
    }
}

void comm_AddMessageStr(char* key, char*(*callback)())
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_STR;
        message->callback.returStr_f = callback;
    }
}

void comm_AddMessageI32(char* key, int32_t(*callback)())
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_I32;
        message->callback.returnI32_f = callback;
    }
}

void comm_AddMessageFloat(char* key, float(*callback)())
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_FLOAT;
        message->callback.returnFloat_f = callback;
    }
}

void comm_AddMessageBool(char* key, bool(*callback)())
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_BOOL;
        message->callback.returnBool_f = callback;
    }
}

void comm_AddMessageJson(char* key, cJSON*(*callback)())
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_CJSON;
        message->callback.returnCjson_f = callback;
    }
}

void comm_AddMessageVoid(char* key)
{
    message_comm_struct_t* message = _AddMessage(key);
    if(message != NULL)
    {
        message->type = COMM_CALLBACK_VOID;
    }
}

static void _Init()
{ 
    nvs_handle_t handle;
    size_t len;
    int32_t interval = 0;
    int8_t isHttpEnabled = 0;
    ESP_LOGI(TAG, "Communication Init:");
    nvs_open("config", NVS_READONLY, &handle);
    if (nvs_get_str(handle,"ServerAddress", NULL, &len) == ESP_OK){
        nvs_get_str(handle, "ServerAddress", _serverAddress, &len);
        ESP_LOGI(TAG, "ServerAddress: %s", _serverAddress);
    }
    if (nvs_get_i32(handle, "CommInterval", &interval) == ESP_OK){
        _commIntervalSec = (uint32_t)interval;
        ESP_LOGI(TAG, "CommInterval: %ld", _commIntervalSec);
    }
    else {
        _commIntervalSec = 60;
    }
    if (nvs_get_i8(handle, "EnableHttp", &isHttpEnabled) == ESP_OK){
        _isHttpEnabled = isHttpEnabled;
        ESP_LOGI(TAG, "EnableHttp: %d", _isHttpEnabled);
    }
    nvs_close(handle);

    comm_AddActionStr("PerformOTA", _PerformOTA);
    comm_AddActionStr("SetModuleId", _SetModuleId);
    comm_AddActionStr("SetModuleName", _SetModuleName);
    comm_AddActionStr("SetModuleKey", _SetModuleKey);

    comm_AddActionInt32("SetCommInterval", _SetCommInterval);
    comm_AddActionStr("SetServerAddress", _SetServerAddress);

    _jsonToSend = cJSON_CreateObject();

    xTaskCreate(_MainLoop, "server_comm", 4096, NULL, tskIDLE_PRIORITY, &_serverCommTask);
    _isInitialized = true;
}

void comm_Start()
{
    if(_isInitialized){
        vTaskResume(_serverCommTask);
    }
    else {
        _Init();
    }
}

void comm_Stop()
{
    vTaskSuspend(_serverCommTask);
}