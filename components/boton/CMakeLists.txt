idf_component_register(SRCS "boton.c"
                        INCLUDE_DIRS "."
                        REQUIRES esp_event driver nvs_flash) 
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 