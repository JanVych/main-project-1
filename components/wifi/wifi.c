#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"

#include "wifi.h"


static const char *TAG = "wifi";
static bool is_sta_connected = true;
static bool is_ap_runnnig = false;
static bool is_wifi_init =  false;

bool isSTAConnected(){
    return is_sta_connected;
}

bool isAPRunnig(){
    return is_ap_runnnig;
}

static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_START){
        is_ap_runnnig = true;
        ESP_LOGI(TAG, "access point start running");
    } 
    else if (event_id == WIFI_EVENT_AP_STOP){
        is_ap_runnnig = false;
        ESP_LOGI(TAG, "access point stop runnig");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED){
        wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
        is_sta_connected = true;
        ESP_LOGI(TAG, "connected to network: %s", event->ssid);
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        is_sta_connected = false;
        ESP_LOGI(TAG, "disconnected from network: %s", event->ssid);
    }
}

static void wifiInit()
{
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifiEventHandler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    
    is_wifi_init = true;
    ESP_LOGI(TAG, "wifi initialized");
}

void wifiAPConfig(uint8_t ssid[32], uint8_t password[64], wifi_auth_mode_t authmode, uint8_t max_connections)
{
    if(!is_wifi_init){
        wifiInit();        
    }
    wifi_config_t wifi_config = {
        .ap = {
            .authmode = authmode,
            .max_connection = max_connections,
            .ssid_len = strlen(&ssid),
        },
    };
    strlcpy(&(wifi_config.ap.ssid), &ssid, 32);
    strlcpy(&(wifi_config.ap.password), &password, 32);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "AP configurated");
}

void wifiAPStart()
{
    if(!is_wifi_init){
        wifiInit();        
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifiAPStop()
{
    esp_wifi_stop();
}


void wifiStartSTA(uint8_t ssid[32], uint8_t password[64])
{
    if(!is_wifi_init){
        wifiInit();        
    }
    wifi_config_t wifi_config;
    wifi_config.sta.failure_retry_cnt = 2;
    strlcpy(&(wifi_config.sta.ssid), &ssid, 32);
    strlcpy(&(wifi_config.sta.password), &password, 32);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifiSTAConnect()
{
    esp_wifi_connect();
}

void wifiDestroy()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    is_wifi_init = false;
    ESP_LOGI(TAG, "wifi destroyed");
}
