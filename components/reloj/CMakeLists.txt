idf_component_register(SRCS "configuracion_hora.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_event esp_timer nvs_flash esp_netif main) 
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 
