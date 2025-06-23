#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_flash.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include <cJSON.h>

#include "program.h"
#include "wifi.h"
#include "http_client.h"
#include "server_comm.h"

#define LED_GPIO 27

static const char *TAG = "main";
static uint8_t s_led_state = 1;

static TaskHandle_t _programTask = NULL;
static bool _isProgramRunning = false;

static char *wifi_ssid = "TP-Link-29";
static char *wifi_password = "***REMOVED***";
// static char *wifi_ssid = "Krepenec2";
// static char *wifi_password = "***REMOVED***";


static void _BlinkLedTask()
{
    s_led_state = !s_led_state;
    gpio_set_level(LED_GPIO, s_led_state);
}

bool _CheckAppState()
{
    return true;
}

static void _RunCheck()
{
    nvs_handle handle;
    uint32_t wait_time = comm_GetIntervalSec() + 120;
    ESP_LOGI(TAG, "Diagnostics started");
    vTaskDelay(wait_time * 1000 / portTICK_PERIOD_MS);
    // for(int i = 0; i < waitTime; i++)
    // {
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     ESP_LOGI(TAG, "Diagnostics in progress, %d seconds left", waitTime - i);
    // }

    if (_CheckAppState()) 
    {
        ESP_LOGI(TAG, "Diagnostics completed successfully");
        // confirm and set new program name
        nvs_open("storage", NVS_READWRITE, &handle);
        size_t len;
        if (nvs_get_str(handle,"_ProgramName", NULL, &len) == ESP_OK)
        {
            char* str = malloc(len);
            nvs_get_str(handle, "_ProgramName", str, &len);
            nvs_set_str(handle, "ProgramName", str);
            nvs_commit(handle);
            free(str);
            nvs_close(handle);
        }
        esp_ota_mark_app_valid_cancel_rollback();
    }
    else 
    {
        ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
    vTaskDelete(NULL);
}

static void _CheckApp()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    esp_err_t result = esp_ota_get_state_partition(running, &ota_state);
    // get state of current running partition
    if (result == ESP_OK) 
    {
        ESP_LOGI(TAG, "current running parttition: %s, in state state: %u", running->label, ota_state);
        // if it is first boot of this partition (after OTA)
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) 
        {
            xTaskCreate( _RunCheck, "checkApp", 2048, NULL, tskIDLE_PRIORITY, NULL );
        }
    }
}

static void _Init()
{
    //initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    //itialize TCP/IP stack.  
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

static void _ProgramRun()
{
    if(!_isProgramRunning && _programTask == NULL){
        xTaskCreate( Main, "program", 20480, NULL, tskIDLE_PRIORITY, &_programTask );
        
    }
    if(!_isProgramRunning && _programTask != NULL){
        vTaskResume(_programTask);
    }
    _isProgramRunning = true;
    
}

static void _ProgramPause()
{
    if(_isProgramRunning){
        vTaskSuspend(_programTask);
        _isProgramRunning = false;
    }
}

static void _ProgramDestroy()
{
    if(_programTask != NULL){
        _isProgramRunning = false;
        OnProgramDestroy();
        vTaskDelete(_programTask);
    }
}

static int32_t _GetProgramStatus()
{
    if(_isProgramRunning)
    {
        return 2;
    }
    return 3;
}

static void _ProgramRestart()
{
    _ProgramDestroy();
    _ProgramRun();
}

void app_main(void)
{

    _Init();

    _CheckApp();

    ESP_LOGI(TAG, "Starting WiFi connection to SSID: %s", wifi_ssid);
    ESP_LOGI(TAG, "WiFi password: %s", wifi_password);
    wifiSTAConnect(wifi_ssid, wifi_password);
    
    comm_AddActionVoid("ProgramRestart", _ProgramRestart);
    comm_AddActionVoid("ProgramRun", _ProgramRun);
    comm_AddActionVoid("ProgramPause", _ProgramPause);

    comm_AddMessageI32("ProgramStatus", _GetProgramStatus);

    comm_Start();
    //commStop();

    _ProgramRun();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    while (true)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        _BlinkLedTask();

    }
}