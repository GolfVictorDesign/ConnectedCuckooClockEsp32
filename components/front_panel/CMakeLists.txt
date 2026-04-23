set(srcs "front_light.cpp" "front_panel.cpp")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_rmt)
                    