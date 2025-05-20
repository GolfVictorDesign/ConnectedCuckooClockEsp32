
idf_component_register(SRCS "wifi_connection.cpp" "${srcs}"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_netif driver esp_wifi vfs nvs_flash)
                    
                    