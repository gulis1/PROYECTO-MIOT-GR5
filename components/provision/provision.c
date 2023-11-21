#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "provision.h"


const static char *TAG = "PROVISION";

const static char* WIFI_SSID;
const static char* WIFI_PASSWORD;

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

        // Se publica el evento con los datos de provisionamiento.
        prov_info_t prov_info = {
            .wifi_ssid = WIFI_SSID,
            .wifi_pass = WIFI_PASSWORD
        };
        esp_event_post(PROVISION_EVENT, PROV_DONE, &prov_info, sizeof(prov_info), portMAX_DELAY);
    } 
    else if (err == ESP_ERR_NOT_FOUND) {
        
        // TODO: aplicación de provisionamiento.
        ESP_LOGW(TAG, "No hay datos de provisionamiento en flash");

        // De momento leemos los datos del menuconfig.
        prov_info_t prov_info = {
            .wifi_ssid = CONFIG_ESP_WIFI_SSID,
            .wifi_pass = CONFIG_ESP_WIFI_PASSWORD
        };
        esp_event_post(PROVISION_EVENT, PROV_DONE, &prov_info, sizeof(prov_info), portMAX_DELAY);
        return ESP_OK;
    }
    else return err;



    return ESP_OK;    
}