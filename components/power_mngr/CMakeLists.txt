idf_component_register(SRCS "power_mngr.c"
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_event esp_timer nvs_flash esp_netif) 
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 