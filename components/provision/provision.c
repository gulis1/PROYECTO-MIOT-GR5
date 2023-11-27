#include <string.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "provision.h"

ESP_EVENT_DEFINE_BASE(PROVISION_EVENT);

const static char *TAG = "PROVISION";
static prov_info_t PROVISION_INFO;

esp_err_t init_provision(void *event_handler) {

    size_t len;
    esp_err_t err;
    nvs_handle_t nvs_handle;

    err = esp_event_handler_register(PROVISION_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_event_handler_register: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_open("nvs", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en nvs_open: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &len);
    if (err == ESP_OK) {

        // Aquí se entra si se encuentran datos de provisionamiento
        // en el NVS.

        PROVISION_INFO.wifi_ssid = malloc(len);
        ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_ssid", PROVISION_INFO.wifi_ssid, &len));

        ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_pass", NULL, &len));
        PROVISION_INFO.wifi_pass = malloc(len);
        ESP_ERROR_CHECK(nvs_get_str(nvs_handle, "wifi_pass", PROVISION_INFO.wifi_pass, &len));
    } 
    else if (err == ESP_ERR_NVS_NOT_FOUND) {
        
        // Aquí se entra si no se encuentran datos de provisionamiento
        // en el NVS.

        // TODO: aplicación de provisionamiento.
        // De forma temporal se leen los datos el menuconfig.
        ESP_LOGW(TAG, "No hay datos de provisionamiento en flash, usando los del menuconfig.");

        PROVISION_INFO.wifi_ssid = malloc(sizeof(CONFIG_ESP_WIFI_SSID));
        strcpy(PROVISION_INFO.wifi_ssid, CONFIG_ESP_WIFI_SSID);

        PROVISION_INFO.wifi_pass = malloc(sizeof(CONFIG_ESP_WIFI_PASSWORD));
        strcpy(PROVISION_INFO.wifi_pass, CONFIG_ESP_WIFI_PASSWORD);
    }
    else {

        // Si hay un error al leer la NVS no se manda el evento.
        ESP_LOGE(TAG, "Error en nvs_get_str: %s", esp_err_to_name(err));
        return err;
    };

    // Se publica el evento con los datos de provisionamiento.
    void *data = &PROVISION_INFO;
    esp_event_post(PROVISION_EVENT, PROV_DONE, &data, sizeof(PROVISION_INFO), portMAX_DELAY);

    return ESP_OK;    
}