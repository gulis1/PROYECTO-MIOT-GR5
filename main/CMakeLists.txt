idf_component_register(SRCS "transiciones.c" "handlers.c" "main.c"
                    INCLUDE_DIRS ".")
component_compile_options(-Wno-error=format= -Wno-format) #para eliminar error log_color 
