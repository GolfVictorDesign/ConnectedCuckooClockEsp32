set(srcs "front_light.c")


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver led_strip)
                    
                    