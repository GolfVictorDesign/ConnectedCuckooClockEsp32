/*
 * wifi_connection.h
 *
 *  Created on: 3 juin 2024
 *      Author: Guillaume Varlet
 */

#ifndef COMPONENTS_WIFI_WIFI_CONNECTION_H_
#define COMPONENTS_WIFI_WIFI_CONNECTION_H_

#if 1

class WifiConnection
{
    protected:
        const static char                   m_log_tag[15];
        const static wifi_init_config_t     m_wifi_config;

        esp_netif_t*        m_pStaNetif;

        esp_event_handler_instance_t    m_instance_any_id;
        esp_event_handler_instance_t    m_instance_got_ip;
        EventGroupHandle_t              m_wifi_event_group;
        EventGroupHandle_t              m_netif_event_group;


        esp_err_t init_wifi(void) { return esp_wifi_init(&m_wifi_config); }
        
        virtual esp_err_t close_connection(void);

    public:
        WifiConnection(void);
        ~WifiConnection(void);
};

class WifiStation : WifiConnection
{
    protected:

    public:
        WifiStation(void);

};
#else
#ifdef __cplusplus
extern "C" {
#endif
    void initWifiConnection(void);


#ifdef __cplusplus
}
#endif
#endif


#endif /* COMPONENTS_WIFI_WIFI_CONNECTION_H_ */
