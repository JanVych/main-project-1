// #include <stdio.h>
// #include <string.h>
// #include "esp_err.h"
// #include "esp_log.h"
// #include "esp_wifi.h"
// #include "esp_mac.h"
// #include "esp_netif.h"
// #include "esp_check.h"

// #include "wifi.h"

// static const char* TAG = "wifi";
// static const char* _staName = "ESP-32";
// static bool _is_wifi_init =  false;
// static wifi_mode_t _current_mode = WIFI_MODE_NAN;

// static void _wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {
//     if (event_id == WIFI_EVENT_AP_STACONNECTED) {
//         wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",MAC2STR(event->mac), event->aid);
//     } 
//     else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
//         wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
//         ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",MAC2STR(event->mac), event->aid);
//     }
//     else if (event_id == WIFI_EVENT_AP_START){
//         ESP_LOGI(TAG, "access point started");
//     } 
//     else if (event_id == WIFI_EVENT_AP_STOP){
//         ESP_LOGI(TAG, "access point stoped");
//     }
//     else if (event_id == WIFI_EVENT_STA_CONNECTED){
//         wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
//         ESP_LOGI(TAG, "connected to network: %s", event->ssid);
//     }
//     else if (event_id == IP_EVENT_STA_GOT_IP){
//         ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
//         _is_sta_connected = true;
//         ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
//     }
//     else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
//         wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
//         _is_sta_connected = false;
//         ESP_LOGI(TAG, "disconnected from network: %s", event->ssid);
//     }
//     else if (event_id == WIFI_EVENT_STA_START){
//         _is_running = true;
//         ESP_LOGI(TAG, "station started");
//     } 
//     else if (event_id == WIFI_EVENT_STA_STOP){
//         _is_running = false;
//         ESP_LOGI(TAG, "station stoped");
//     }
// }

// esp_err_t void _WififInit()
// {
//     esp_netif_create_default_wifi_ap();
//     esp_netif_t *sta = esp_netif_create_default_wifi_sta();
//     ESP_RETURN_ON_ERROR(esp_netif_set_hostname(sta, _staName));
//     esp_netif_create_default_wifi_ap();

//     ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
//                                         ESP_EVENT_ANY_ID,
//                                         &_wifiEventHandler,
//                                         NULL,
//                                         NULL));
//     ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
//                                         IP_EVENT_STA_GOT_IP,
//                                         &_wifiEventHandler,
//                                         NULL,
//                                         NULL));

//     wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_RETURN_ON_ERROR(esp_wifi_init(&config));
//     _is_wifi_init = true;
//     ESP_LOGI(TAG, "wifi initialized");
// }

// esp_err_t void _WifiStaStart(char* ssid, char* password)
// {
//     if(!_is_wifi_init){
//         ESP_RETURN_ON_ERROR(_WififInit());        
//     }
//     wifi_config_t wifi_config = 
//     {
//         .sta = {
//             .ssid = ssid,
//             .password = password
//         }
//     };
//     //wifi_config.sta.failure_retry_cnt = 2;
    
//     ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), "Error");
//     ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_RETURN_ON_ERROR(esp_wifi_start());
// }

// esp_err_t _WifiApStart()
// {
//     if(!_is_wifi_init){
//         ESP_RETURN_ON_ERROR(_WififInit());        
//     }
//     wifi_config_t wifi_config = {
//         .ap = {
//         }
//     }
// }
//     ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP));
//     ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_MODE_AP, &wifi_config));
//     ESP_RETURN_ON_ERROR(esp_wifi_start());
// }

// esp_err_t wifi_Connect(char* ssid, char* password)
// {
//     if(_current_mode == WIFI_MODE_STA)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_disconnect());
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//     }
//     else if(_current_mode == WIFI_MODE_AP)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//     }
//     _current_mode = WIFI_MODE_STA;
//     ESP_RETURN_ON_ERROR(_WifiStaStart(ssid, password));
// }

// esp_err_t wifi_Disconnect()
// {
//     if(_current_mode == WIFI_MODE_STA)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_disconnect())
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//         _current_mode == WIFI_MODE_NAN;
//     }
// }

// esp_err_t wifi_Open()
// {
//     else if(_current_mode == WIFI_MODE_STA)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_disconnect())
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//     }
//     else if(_current_mode == WIFI_MODE_AP)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//     }
//     _current_mode = WIFI_MODE_AP;
//     ESP_RETURN_ON_ERROR(_WifiApStart());
// }

// esp_err_t wifi_Close()
// {
//     if(_current_mode == WIFI_MODE_AP)
//     {
//         ESP_RETURN_ON_ERROR(esp_wifi_stop());
//         _current_mode == WIFI_MODE_NAN;
//     }
// }

// wifi_mode_t wifi_GetMode()
// {
//     return _current_mode;
// }

// bool wifi_IsStaConnected()
// {
//     wifi_ap_record_t ap_info;
//     if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
//         return true;
//         // ESP_LOGI(TAG, "Connected to Wi-Fi SSID: %s", ap_info.ssid);
//     } else {
//         // ESP_LOGI(TAG, "Not connected to Wi-Fi");
//         return false;
//     }
// }
