//
// Created by Rasanja Dampriya on 8/21/25.
//

#include "wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "sdkconfig.h"  // brings in CONFIG_* values

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define MAX_RETRY           10

/** GLOBALS **/
static const char *TAG = "WIFI"; // task tag
static EventGroupHandle_t s_wifi_event_group; // event group to contain status information
static int s_retry_num = 0; // retry tracker

/** FUNCTIONS **/
//event handler for wifi events
static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            s_retry_num++;
            ESP_LOGW(TAG, "Reconnecting to AP... attempt %d", s_retry_num);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
}

//event handler for ip events
static void ip_event_handler(void* arg,
                             esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* e = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// connect to wifi and return the result
esp_err_t wifi_init_and_connect(void)
{
    esp_err_t err;

    // One-time network + event loop init
    ESP_ERROR_CHECK(esp_netif_init()); //initialize the esp network interface
    ESP_ERROR_CHECK(esp_event_loop_create_default()); //initialize default esp event loop
    esp_netif_create_default_wifi_sta(); //create wifi station in the wifi driver

    //setup wifi station with the default wifi configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /** EVENT LOOP CRAZINESS **/
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL, NULL));

    /** START THE WIFI DRIVER **/
    wifi_config_t wifi_config = { 0 };

    // Fill SSID/pass from sdkconfig
    strncpy((char*)wifi_config.sta.ssid,     CONFIG_WIFI_SSID,     sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // set the wifi controller to be a station
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // set the wifi config
    ESP_ERROR_CHECK(esp_wifi_start()); // start the wifi driver

    ESP_LOGI(TAG, "STA init complete, waiting for IP...");

    /** NOW WE WAIT **/
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
* happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID:%s", CONFIG_WIFI_SSID);
        err = ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", CONFIG_WIFI_SSID);
        err = ESP_FAIL;
    }

    // Handlers remain registered to catch reconnects. Do not delete the group yet.
    return err;
}