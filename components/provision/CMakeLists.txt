idf_component_register(SRCS "provision.c"
                    INCLUDE_DIRS "." "../managment"
                    REQUIRES esp_event nvs_flash esp_wifi wifi_provisioning managment)
