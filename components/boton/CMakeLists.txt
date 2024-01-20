idf_component_register(SRCS "boton.c"
                        INCLUDE_DIRS "."
                        REQUIRES esp_event main) 
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 