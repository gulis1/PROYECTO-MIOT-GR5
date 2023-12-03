#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>

#include "mqtt_api.h"

const static char *TAG = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;

esp_err_t mqtt_init(void *event_handler, char *device_token) {

    esp_err_t err;
    char mqtt_url[256];

    if (snprintf(mqtt_url, sizeof(mqtt_url), "mqtt://%s", CONFIG_THINGSBOARD_URL) > sizeof(mqtt_url)) {
        ESP_LOGE(TAG, "mqtt broker URL too large");
        return ESP_ERR_NO_MEM;
    }

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = mqtt_url,
        .credentials.username = device_token,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Error: esp_mqtt_client_init");
        return ESP_ERR_INVALID_ARG;
    }

    // Registramos los eventos de base MQTT_EVENTS al handler.
    err = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, event_handler, mqtt_client);
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

esp_err_t mqtt_send(char *topic, char *data, int qos) {
    
    if (esp_mqtt_client_publish(mqtt_client, topic, data, 0, qos, 0) != -1)
        return ESP_OK;
    else
        return ESP_FAIL;  
}

esp_err_t mqtt_subscribe(char *topic) {
    return esp_mqtt_client_subscribe(mqtt_client, topic, 2) != -1 ? ESP_OK : ESP_FAIL;
}