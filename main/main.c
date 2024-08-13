#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_flash.h"
#include "esp_https_ota.h"
#include <cJSON.h>

#include "program.h"
#include "wifi.h"
#include "http_client.h"
#include "server_comm.h"

#define LED_GPIO 2

static const char *TAG = "main";
static uint8_t s_led_state = 1;

static char *wifi_ssid = "TP-Link-29";
static char *wifi_password = "***REMOVED***";


static void blink_led(void)
{
    s_led_state = !s_led_state;
    gpio_set_level(LED_GPIO, s_led_state);
}

static void init()
{
    //initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    //itialize TCP/IP stack.  
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

static void defaultCallback(char* str){
    ESP_LOGI(TAG ,"callback with arg: %s", str);
}

void app_main(void)
{
    init();

    xTaskCreate( program, "program", 2048, NULL, tskIDLE_PRIORITY, NULL );

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    wifiSTAConnect(wifi_ssid, wifi_password);

    vTaskDelay(4000 / portTICK_PERIOD_MS); 

        ESP_LOGI(TAG, "Starting OTA example task");
    esp_http_client_config_t config = 
    {
        .url = "http://192.168.0.105:45455/api/firmware",
        .keep_alive_enable = true,
        .cert_pem = NULL,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "waiting old version");
    }
    
    // http_response_t data_get = {};
    // char *url = "http://192.168.0.103:45455/api/modules";
    // //char *url = "http://jsonplaceholder.typicode.com/todos/1";
    // httpGet(url, &data_get);
    // ESP_LOGI(TAG, "recived data: %.*s", data_get.data_len, data_get.data);

    // http_response_t data_post= {};
    // char *data = "{\"fields\":{\"Memory\":{\"doubleValue\":\"2\"},\"Name\":{\"stringValue\":\"Data from the client\"}}}";
    // httpPost(url, data, &data_post);
    // ESP_LOGI(TAG, "recived data: %.*s", data_post.data_len, data_post.data);

    // httpGetJson(url, &data_get);
    // ESP_LOGI(TAG, "recived data: %s", cJSON_Print(data_get.json));

    // httpPostJson(url, cJSON_Parse(data), &data_post);
    // ESP_LOGI(TAG, "recived data: %.*s", data_post.data_len, data_post.data);
    // ESP_LOGI(TAG, "type : %s", data_post.content_type);

    // free(data_post.data);
    // free(data_post.content_type);

    // httpPostJson(url, cJSON_Parse(data), &data_post);
    // ESP_LOGI(TAG, "recived data: %.*s", data_post.data_len, data_post.data);
    // ESP_LOGI(TAG, "type : %s", data_post.content_type);

    commStart();
    commAddAction("test-main", defaultCallback);

    // char *ssid;
    // char *networks;
    while (true)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();

        // ssid = wifiGetSTASsid();
        // ESP_LOGI(TAG, "SSID: %s", ssid);
        // free(ssid);
        
        // vTaskDelay(6000 / portTICK_PERIOD_MS);
        
        // networks = wifiGetAvailableNetworks();
        // ESP_LOGI(TAG, "available networks: %s", networks);
        // free(networks);

        // commStop();
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        // commStart();
        //wifiSTADisconnect();
        
    }
    
}