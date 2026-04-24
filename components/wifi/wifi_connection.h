/*
 * wifi_connection.h
 *
 *  Created on: 3 juin 2024
 *      Author: Guillaume Varlet
 */

#ifndef COMPONENTS_WIFI_WIFI_CONNECTION_H_
#define COMPONENTS_WIFI_WIFI_CONNECTION_H_

#include "esp_wifi.h"


class WifiConnection
{    
    protected:
        const static char               m_logTag[15];
        const static wifi_init_config_t m_wifiConfig;

        esp_netif_t*                    m_pStaNetif;

        esp_event_handler_instance_t    m_instanceAnyId;
        esp_event_handler_instance_t    m_instanceGotIp;
        EventGroupHandle_t              m_wifiEventGroup;
        EventGroupHandle_t              m_netifEventGroup;
        
        static void wifi_event_handler(
                                    void* arg, 
                                    esp_event_base_t event_base,
                                    int32_t event_id, 
                                    void* event_data);

        void ip_event_handler(
                            void* arg, 
                            esp_event_base_t event_base,
                            int32_t event_id, 
                            void* event_data);

        static void ip_event_handler_thunk(
                                            void* arg, 
                                            esp_event_base_t event_base,
                                            int32_t event_id, 
                                            void* event_data);

        esp_err_t init_wifi(void) { return esp_wifi_init(&m_wifiConfig); }
        
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


#endif /* COMPONENTS_WIFI_WIFI_CONNECTION_H_ */
