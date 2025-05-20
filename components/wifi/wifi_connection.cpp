/*
 * wifi_connection.cpp
 *
 *  Created on: 30 mai 2024
 *      Author: Guillaume Varlet
 */

#include <string>
#include <functional>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sdkconfig.h"

#include "wifi_connection.h"
#include "wifi_connection_settings.h"


 /* 
  * The event group allows multiple bits for each event, but we only care about two events:
  * - we are connected to the AP with an IP
  * - we failed to connect after the maximum amount of retries 
  */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**********************************************************************************************************************
 *                                              Constants declarations                                                *
 **********************************************************************************************************************/
const char WifiConnection::m_log_tag[15] = "WifiConnection";
const wifi_init_config_t WifiConnection::m_wifi_config = WIFI_INIT_CONFIG_DEFAULT();


static void wifi_event_handler(
    void* arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void* event_data)
{
    auto check_wifi_connect_error = [](esp_err_t result)
    {
        switch (result) {
        case ESP_OK:
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            ESP_LOGE("wifi_event_handler", "WiFi is not correctly initalized");
            break;

        case ESP_ERR_WIFI_NOT_STARTED:
            ESP_LOGE("wifi_event_handler", "WiFi is not started");
            break;

        case ESP_ERR_WIFI_CONN:
            ESP_LOGE("wifi_event_handler", "WiFi internal error, station control block wrong");
            break;

        case ESP_ERR_WIFI_SSID:
            ESP_LOGE("wifi_event_handler", "SSID of AP which station connects is invalid");
            break;

        default:
            break;
        }
    };

    static uint8_t num_retry = 0;
    EventGroupHandle_t*  p_wifi_event_group;
    esp_err_t result;

    p_wifi_event_group = (EventGroupHandle_t*)event_data;

    switch (event_id)
    {
        case WIFI_EVENT_STA_START:
            result = esp_wifi_connect();
            check_wifi_connect_error(result);
            break;
        
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI("wifi_event_handler", "Connected to the AP");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            if (num_retry < CONFIG_WIFI_MAXIMUM_RETRY) {
                result = esp_wifi_connect();
                check_wifi_connect_error(result);
                ESP_LOGI("wifi_event_handler", "retry to connect to the AP");
            }
            else {
                xEventGroupSetBits(*p_wifi_event_group, WIFI_FAIL_BIT);
            }

            ESP_LOGI("wifi_event_handler", "disconnect to the AP fail");
            break;
        
        default:
            ESP_LOGI("wifi_event_handler", "Got event %d", (int)event_id);
            break;
    }
}

static void netif_event_handler(
    void* arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void* event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI("netif_event_handler", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        //xEventGroupSetBits(*p_event_group, WIFI_CONNECTED_BIT);
    }
}


WifiConnection::WifiConnection(void)
{
    esp_err_t result;
    m_pStaNetif = nullptr;

    /*
     * Initialize Non-Volatile Storage Library as the WiFi interface requires this to run 
     */
    result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);

    /*
     * Create a WiFi and netif event groups
     */
    m_wifi_event_group = xEventGroupCreate();
    m_netif_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    result = esp_event_loop_create_default();
    switch (result)
    {
        case ESP_ERR_NO_MEM:
            ESP_LOGE(m_log_tag, "Cannot create default event loop");
            break;

        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(m_log_tag, "Event loop already started");
            break;

        case ESP_FAIL:
            ESP_LOGE(m_log_tag, "Failed to create default event loop task");
            break;

        default:
                break;
    }
}

esp_err_t WifiConnection::close_connection(void)
{
    esp_err_t ret;

    ret = esp_event_loop_delete_default();
    ESP_ERROR_CHECK(ret);

    esp_wifi_deinit();
    esp_wifi_clear_default_wifi_driver_and_handlers(m_pStaNetif);
    esp_netif_destroy(m_pStaNetif);

    /*
     * Deinitialize Non-Volatile Storage Library as the WiFi interface requires this to run
     */
    ret = nvs_flash_deinit();
    ESP_ERROR_CHECK(ret);

    return (ret);
}

WifiConnection::~WifiConnection(void)
{
    close_connection();
    m_pStaNetif = nullptr;
}

/***********************************************************************************************************************
 *                                                WiFi Station class                                                   *
 **********************************************************************************************************************/

WifiStation::WifiStation(void)
{
    auto check_event_register_error = [] (esp_err_t result)
    {
        switch (result)
        {
            case ESP_OK:
                break;

            case ESP_ERR_NO_MEM: 
                ESP_LOGE("WIFI station start", "Cannot allocate memory for the handler");
                break;
        }
    };

    esp_err_t result;
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .bssid_set = false,                         /* No need to check MAC address of AP */
            .channel = 0,                               /* AP channel is unknown */
            .listen_interval = 10,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {
                .rssi = -127,
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
            .pmf_cfg = {
                .capable = false,
                .required = false,
            },
            .sae_pwe_h2e = (wifi_sae_pwe_method_t)WPA3_SAE_PWE_UNSPECIFIED,
            .sae_pk_mode = WPA3_SAE_PK_MODE_DISABLED,
            .failure_retry_cnt = CONFIG_WIFI_MAXIMUM_RETRY,
            .sae_h2e_identifier = "",
        },
    };

    m_pStaNetif = esp_netif_create_default_wifi_sta();
    result = init_wifi();
    ESP_ERROR_CHECK(result);

    /* Register all event required for WiFi connection*/
    result = esp_event_handler_instance_register(
        WIFI_EVENT, 
        ESP_EVENT_ANY_ID,
        &wifi_event_handler, 
        &m_wifi_event_group,
        &m_instance_any_id);
    check_event_register_error(result);
    
    result = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &netif_event_handler,
        NULL,
        &m_instance_got_ip);
    check_event_register_error(result);

    ESP_LOGI("WIFI station event", "Registered events");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
}
