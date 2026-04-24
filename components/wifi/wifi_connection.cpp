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
#include "esp_log_level.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"

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
const char WifiConnection::m_logTag[15] = "WifiConnection";
const wifi_init_config_t WifiConnection::m_wifiConfig = WIFI_INIT_CONFIG_DEFAULT();


void WifiConnection::wifi_event_handler(
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
            ESP_LOGE(m_logTag, "WiFi is not correctly initialized");
            break;

        case ESP_ERR_WIFI_NOT_STARTED:
            ESP_LOGE(m_logTag, "WiFi is not started");
            break;

        case ESP_ERR_WIFI_CONN:
            ESP_LOGE(m_logTag, "WiFi internal error, station control block wrong");
            break;

        case ESP_ERR_WIFI_SSID:
            ESP_LOGE(m_logTag, "SSID of AP which station connects is invalid");
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
            ESP_LOGI(m_logTag, "Connected to the AP");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            if (num_retry < CONFIG_WIFI_MAXIMUM_RETRY) {
                result = esp_wifi_connect();
                check_wifi_connect_error(result);
                ESP_LOGI(m_logTag, "retry to connect to the AP");
            }
            else {
                xEventGroupSetBits(*p_wifi_event_group, WIFI_FAIL_BIT);
            }

            ESP_LOGI(m_logTag, "disconnect to the AP fail");
            break;
        
        default:
            ESP_LOGI(m_logTag, "Got event %d", (int)event_id);
            break;
    }
}

void WifiConnection::ip_event_handler_thunk(
                                            void* arg, 
                                            esp_event_base_t event_base,
                                            int32_t event_id, 
                                            void* event_data )
{
    WifiConnection* instance = static_cast<WifiConnection*>(arg);
    instance->ip_event_handler(nullptr, event_base, event_id, event_data);
}

void WifiConnection::ip_event_handler(
                                    void* arg, 
                                    esp_event_base_t event_base,
                                    int32_t event_id, 
                                    void* event_data )
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(m_logTag, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
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
    m_wifiEventGroup = xEventGroupCreate();
    m_netifEventGroup = xEventGroupCreate();
    result = esp_event_loop_create_default();

    ESP_ERROR_CHECK(esp_netif_init());

    switch (result)
    {
        case ESP_ERR_NO_MEM:
            ESP_LOGE(m_logTag, "Cannot create default event loop");
            break;

        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(m_logTag, "Event loop already started");
            break;

        case ESP_FAIL:
            ESP_LOGE(m_logTag, "Failed to create default event loop task");
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
            .bssid = "",
            .channel = 0,                               /* AP channel is unknown */
            .listen_interval = 10,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {
                .rssi = -127,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .rssi_5g_adjustment = 0
            },
            .pmf_cfg = {
                .capable = false,
                .required = false,
            },
            .rm_enabled = 1,
            .btm_enabled = 1,
            .mbo_enabled = 1,
            .ft_enabled =1,
            .owe_enabled = 0,
            .transition_disable = 1,
            .disable_wpa3_compatible_mode = 1,
            .reserved1 = 25,
            .sae_pwe_h2e = (wifi_sae_pwe_method_t)WPA3_SAE_PWE_UNSPECIFIED,
            .sae_pk_mode = WPA3_SAE_PK_MODE_DISABLED,
            .failure_retry_cnt = CONFIG_WIFI_MAXIMUM_RETRY,
            .he_dcm_set = 1,
            .he_dcm_max_constellation_tx = 2,
            .he_dcm_max_constellation_rx = 2,
            .he_mcs9_enabled = 1,
            .he_su_beamformee_disabled = 1,
            .he_trig_su_bmforming_feedback_disabled = 1,
            .he_trig_mu_bmforming_partial_feedback_disabled = 1,
            .he_trig_cqi_feedback_disabled = 1,
            .vht_su_beamformee_disabled = 1,
            .vht_mu_beamformee_disabled = 1,
            .vht_mcs8_enabled = 1,
            .reserved2 = 19,
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
        &m_wifiEventGroup,
        &m_instanceAnyId);
    check_event_register_error(result);
    
    result = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &ip_event_handler_thunk,
        this,
        &m_instanceGotIp);
    check_event_register_error(result);

    ESP_LOGI("WIFI station event", "Registered events");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
}
