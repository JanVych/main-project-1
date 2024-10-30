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
//#include "etatherm.h"

#define LED_GPIO 2

static const char *TAG = "main";
static uint8_t s_led_state = 1;

static TaskHandle_t program_task;

static char *wifi_ssid = "TP-Link-29";
static char *wifi_password = "***REMOVED***";


static void _blinkLed(void)
{
    s_led_state = !s_led_state;
    gpio_set_level(LED_GPIO, s_led_state);
}

static void _programStart()
{
    xTaskCreate( program, "program", 20480, NULL, tskIDLE_PRIORITY, &program_task );
}

static void _check_app()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    nvs_handle handle;
    // get state of current running partition
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) 
    {
        // first boot of this partition after OTA
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) 
        {
            ///////////// !!!
            bool is_ok = true;
            if (is_ok) 
            {
                ESP_LOGI(TAG, "Diagnostics completed successfully");
                // confirm and set new program name
                nvs_open("storage", NVS_READONLY, &handle);
                uint32_t len;
                if (nvs_get_str(handle,"ProgramName", NULL, &len) == ESP_OK){
                    char* str = malloc(len);
                    nvs_get_str(handle, "_ProgramName", str, &len);
                    nvs_set_str(handle, "ProgramName", str);
                    nvs_commit(handle);
                    free(str);
                    nvs_close(handle);
                }
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
}

static void _init()
{
    //initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    //itialize TCP/IP stack.  
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void app_main(void)
{
    _init();

    _check_app();

    wifiSTAConnect(wifi_ssid, wifi_password);

    commStart();
    //commStop();

    _programStart();

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    while (true)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        _blinkLed();

    }
}