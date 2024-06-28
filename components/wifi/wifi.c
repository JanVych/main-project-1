#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"

#include "wifi.h"


static const char *TAG = "wifi";

static uint8_t ap_ssid[32] = "ESP-32";
static uint8_t ap_password[64] = "";
static wifi_auth_mode_t ap_authmode = WIFI_AUTH_OPEN;
static uint8_t ap_max_connections = 16;

static uint8_t is_wifi_init =  false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    // if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    //     ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    //     wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    //     ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // }
}



static void wifiInit()
{
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    is_wifi_init = true;
}


void wifiStartNewAP(uint8_t ssid[32], uint8_t password[64], wifi_auth_mode_t authmode, uint8_t max_connections)
{
    if(!is_wifi_init){
        wifiInit();        
    }
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = *ap_ssid,
            .ssid_len = strlen("sa"),
            .password = *ap_password,
            .max_connection = ap_max_connections,
            .authmode = ap_authmode,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_softap finished");

}

void wifiStartAP()
{
    wifiStartNewAP(ap_ssid, ap_password, ap_authmode, ap_max_connections);
}
