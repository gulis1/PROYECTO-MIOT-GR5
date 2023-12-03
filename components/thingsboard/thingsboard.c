#include <esp_log.h>
#include <esp_event.h>
#include <esp_event.h>

#include "mqtt_api.h"
#include "coap_client.h"
#include "thingsboard.h"


const char *TAG = "thingsboard";
ESP_EVENT_DEFINE_BASE(THINGSBOARD_EVENT);

#ifdef CONFIG_USE_MQTT
void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    switch (event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado al broker MQTT de Thingsboard.");
            esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_DISCONNECTED:
            esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
            break;
    }
}
#endif


esp_err_t thingsboard_init(void *handler) {

    esp_err_t err;
    err = esp_event_handler_register(THINGSBOARD_EVENT, ESP_EVENT_ANY_ID, handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_event_handler_register: %s", esp_err_to_name(err));
        return err;
    }

    #if CONFIG_USE_COAP
        err = coap_client_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en coap_server_init: %s", esp_err_to_name(err));
            return err;
        }
    #elif CONFIG_USE_MQTT
        // Iniciación MQTT. Se le pasa el handler de los eventos
        // MQTT para que se registre. 
        err = mqtt_init(mqtt_handler);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en mqtt_api_init: %s", esp_err_to_name(err));
            return err;
        }
    #else
        ESP_LOGE(TAG, "Protocolo mal configurado")
        return ESP_FAIL;
    #endif

        return ESP_OK;
}

esp_err_t thingsboard_start() {

    #if CONFIG_USE_COAP
        // COAP no requiere conexión, por lo que lanzamos directamente
        // el evento de ready.
        return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
    #elif CONFIG_USE_MQTT
        // Iniciamos la conexión al broker MQTT.
        return mqtt_start();
    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_telemetry_send(char *msg) {

    #if CONFIG_USE_COAP
        return coap_client_telemetry_send(msg);
    #elif CONFIG_USE_MQTT
        return mqtt_send("v1/devices/me/telemetry", msg, 0);

    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_stop() {

    #ifdef CONFIG_USE_COAP
        return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
    #elif CONFIG_USE_MQTT
        return mqtt_stop();
    #else
        return ESP_FAIL;
    #endif

}
