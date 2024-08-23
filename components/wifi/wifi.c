#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"

#include "wifi.h"

static const char *TAG = "wifi";
static bool is_wifi_init =  false;

static bool is_running = false;

static bool is_sta_connected = false;

static char sta_ssid[32];
const char *sta_name = "ESP-32";

char* wifiGetSTASsid()
{
    if (is_sta_connected)
    {
        char* ssid = malloc(33);
        strlcpy(ssid, sta_ssid, 33);
        return ssid;
    }
    return NULL;
}

char* wifiGetAvailableNetworks()
{
    if (is_running)
    {
        ESP_LOGI(TAG, "wifi scan started");
        //wifi_scan_config_t config = { .show_hidden = false };
        esp_wifi_scan_start(NULL, true);

        uint16_t records_count = 0;
        esp_wifi_scan_get_ap_num(&records_count);

        wifi_ap_record_t records [records_count];
        esp_wifi_scan_get_ap_records(&records_count, records);
        
        uint32_t total_length = 0;
        for (int i = 0; i < records_count; i++) {
            total_length += strlen((char*)records[i].ssid) + 1;
        }
        char *records_str = malloc(total_length);

        records_str[0] = '\0';
        records_str[total_length - 1] = '\0';
        for (int i = 0; i < records_count; i++) {
            strcat(records_str, (char*)records[i].ssid);
            if (i < records_count - 1) {
                strcat(records_str, ",");
            }
        }
        ESP_LOGI(TAG, "wifi scan stoped, found: %u records", records_count);
        return records_str;
    }
    return NULL;
}

bool wifiIsSTAConnected(){
    return is_sta_connected;
}

bool wifiIsRunning(){
    return is_running;
}

static void _wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
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
        ESP_LOGI(TAG, "access point started");
    } 
    else if (event_id == WIFI_EVENT_AP_STOP){
        ESP_LOGI(TAG, "access point stoped");
    }
    else if (event_id == WIFI_EVENT_STA_CONNECTED){
        wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
        ESP_LOGI(TAG, "connected to network: %s", event->ssid);
    }
    else if (event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        is_sta_connected = true;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        is_sta_connected = false;
        ESP_LOGI(TAG, "disconnected from network: %s", event->ssid);
    }
    else if (event_id == WIFI_EVENT_STA_START){
        is_running = true;
        ESP_LOGI(TAG, "station started");
    } 
    else if (event_id == WIFI_EVENT_STA_STOP){
        is_running = false;
        ESP_LOGI(TAG, "station stoped");
    }
}

static void _wifiInit()
{
    esp_netif_create_default_wifi_ap();
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta, sta_name);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &_wifiEventHandler,
                                        NULL,
                                        NULL);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &_wifiEventHandler,
                                        NULL,
                                        NULL);

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    
    is_wifi_init = true;
    ESP_LOGI(TAG, "wifi initialized");
}

// Access point //
// void wifiAPConfig(char* ssid, char* password, wifi_auth_mode_t authmode, uint8_t max_connections)
// {
//     if(!is_wifi_init){
//         _wifiInit();        
//     }
//     wifi_config_t wifi_config = {
//         .ap = {
//             .authmode = authmode,
//             .max_connection = max_connections,
//             .ssid_len = strlen(ssid),
//         },
//     };

//     strlcpy((char*)wifi_config.ap.ssid, ssid, 32);
//     strlcpy((char*)wifi_config.ap.password, password, 32);

//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
//     ESP_LOGI(TAG, "AP configurated");
// }

// void wifiAPStart()
// {
//     if (!is_running)
//     {
//         if(!is_wifi_init){
//             _wifiInit();        
//         }
//         ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//         ESP_ERROR_CHECK(esp_wifi_start());
//     }
// }

// void wifiAPStop()
// {
//     if(is_running)
//     {
//         esp_wifi_stop();
//     }  
// }

// Station //

static void _wifiSTAStart(char *ssid, char *password)
{
    if(!is_wifi_init){
        _wifiInit();        
    }
    strncpy(sta_ssid, ssid, 32);
    wifi_config_t wifi_config = {
        .sta = {}
    }; 
    //wifi_config.sta.failure_retry_cnt = 2;
    strncpy((char*)wifi_config.sta.ssid, ssid, 32);
    strncpy((char*)wifi_config.sta.password, password, 64);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifiSTAConnect(char *ssid, char *password)
{
    if (!is_running){
        _wifiSTAStart(ssid, password);
    }
    wifiSTADisconnect();
    esp_wifi_connect();
}

void wifiSTADisconnect()
{
    if(is_sta_connected){
        esp_wifi_disconnect();
    }
}

// Remove wifi driver // 
void wifiDestroy()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    is_wifi_init = false;
    ESP_LOGI(TAG, "wifi destroyed");
}
