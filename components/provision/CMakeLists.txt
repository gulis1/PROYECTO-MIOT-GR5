idf_component_register(SRCS "provision.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_event nvs_flash esp_wifi wifi_provisioning)
