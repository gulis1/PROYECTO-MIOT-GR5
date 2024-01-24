idf_component_register(SRCS "i2c_config.c" "sensores.c" "si7021.c" "SGP30.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_event esp_timer) 
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 
