#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>

#include "mqtt_api.h"

const static char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;


esp_err_t mqtt_init(void *mqtt_handler) {

    esp_err_t err;

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

    // Registramos los eventos de base MQTT_EVENTS al handler.
    err = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_handler, mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_mqtt_client_register_event: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t mqtt_start() {
    return esp_mqtt_client_start(mqtt_client);
}

esp_err_t mqtt_stop() {
    return esp_mqtt_client_stop(mqtt_client);
}