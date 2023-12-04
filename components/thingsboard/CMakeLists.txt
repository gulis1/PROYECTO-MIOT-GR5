idf_component_register(SRCS "thingsboard.c" "mqtt_api.c" "coap_client.c"
                    INCLUDE_DIRS .
                    REQUIRES esp_event esp_timer espressif__coap mqtt nvs_flash json)
