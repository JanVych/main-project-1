#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_app_desc.h"
#include "nvs.h"
#include <cJSON.h>

#include "program.h"
#include "wifi.h"
#include "http_client.h"

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
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    //itialize TCP/IP stack.  
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

static void printInfo()
{
    nvs_handle_t handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    int64_t id;
    size_t len;
    nvs_get_i64(handle, "id", &id);
    nvs_get_str(handle,"module_name", NULL, &len);
    char* name = malloc(len);
    nvs_get_str(handle, "module_name", name, &len);

    printf("\nId: %lld", id);
    printf("\nModule name: %s", name);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("\nThis is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? "/IEEE802.15.4" : "");

    printf("silicon revision %d, ", chip_info.revision);

    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }
    printf("%luMB %s flash", flash_size / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    const esp_app_desc_t *desc = esp_app_get_description();
    printf("\napp version: %s", desc->version);
    printf("\nproject name: %s", desc->project_name);
    printf("\nsdk version: %s", desc->idf_ver);
    printf("\nfree heap: %fKB\n\n", (double)esp_get_free_heap_size() / 1000.0);
}

void app_main(void)
{
    init();

    uint8_t ucParameterToPass;

    xTaskCreate( program, "program", 2048, &ucParameterToPass, tskIDLE_PRIORITY, NULL );

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    wifiSTAConnect(wifi_ssid, wifi_password);

    // nvs_handle_t handle;
    // nvs_open("storage", NVS_READWRITE, &handle);
    // nvs_set_i64(handle, "Id", 1);
    // nvs_set_str(handle, "ModuleName", "ESP32-module1");
    // nvs_commit(handle);
    // nvs_close(handle);

    printInfo();

    // vTaskDelay(4000 / portTICK_PERIOD_MS); 
    
    // http_response_t data_get = {};
    // char *url = "http://192.168.0.101:45455/api/modules";
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

    while (true)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        
        //vTaskDelay(10000 / portTICK_PERIOD_MS);
        
        //wifiSTADisconnect();
        
    }
    
}