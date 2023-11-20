idf_component_register(SRCS "mqtt_api.c"
                    INCLUDE_DIRS .
                    REQUIRES esp_event esp_timer mqtt)
