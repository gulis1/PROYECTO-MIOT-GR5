#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "provision.h"


const static char *TAG = "PROVISION";

ESP_EVENT_DEFINE_BASE(PROVISION_EVENT);

esp_err_t init_provision(void *mqtt_handler) {

    size_t len;
    esp_err_t err;
    char *ssid, *password;
    nvs_handle_t nvs_handle;

    nvs_open("nvs", NVS_READWRITE, &nvs_handle);

    err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &len);
    if (err == ESP_OK) {

        ESP_ERROR_CHECK(nvs_get_str(&nvs_handle, "wifi_ssid", &ssid, &len));
        ESP_ERROR_CHECK(nvs_get_str(&nvs_handle, "wifi_pass", NULL, &len));
        ESP_ERROR_CHECK(nvs_get_str(&nvs_handle, "wifi_pass", &password, &len));

        // prov_info_t
        // esp_event_post(PROVISION_EVENT, PROV_DONE, )
    } 
    else if (err == ESP_ERR_NOT_FOUND) {

    }



    return ESP_OK;    
}