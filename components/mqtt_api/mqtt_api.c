#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>

#include "mqtt_api.h"

const static char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;


esp_err_t mqtt_api_init() {

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URL,
        .credentials.username = CONFIG_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD,       
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Error: esp_mqtt_client_init");
        return ESP_ERR_INVALID_ARG;
    }
    return esp_mqtt_client_start(mqtt_client);
}