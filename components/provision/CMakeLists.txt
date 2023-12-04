idf_component_register(SRCS "provision.c"
                    INCLUDE_DIRS "." "./managed_components" "./managed_components/espressif__qrcode/include"
                    REQUIRES esp_event nvs_flash esp_wifi wifi_provisioning)
