#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_wifi.h>
//#include <protocol_examples_common.h> // TODO: quitar esto cuando tengamos el wifi bueno.
#include "main.h"
#include "mqtt_api.h"
#include "wifi.h"

const static char* TAG = "main.c";

static QueueHandle_t queue;

void main_task() {

    estado_t estado_actual = ESTADO_PLACEHOLDER;

    while (true) {

        transicion_t transicion;
        if (xQueueReceive(queue, &transicion, portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Error en xQueueReceive.");
            continue;
        }

        switch (estado_actual) {

            default:
                ESP_LOGE(TAG, "Estado desconocido: %d.", estado_actual);
        }
    }
}

// Handler donde se recibirán y procesarán los eventos de todos los componentes.
void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {


    // TODO: poner el switch con los eventos.
    ESP_LOGI(TAG, "Recibido evento de base: %s e id: %ld", event_base, event_id);

}

void app_main(void) {

    esp_err_t err;

    // Iniciación flash.
    err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en nvs_flash_init: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Connecting to wifi...");
    err = wifi_init_sta();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en nvs_flash_init: %s", esp_err_to_name(err));
        return;
    }

    // Creación del default event loop.
    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_event_loop_create_default: %s", esp_err_to_name(err));
        return;
    }

    // Creación de la cola.
    queue = xQueueCreate(16, sizeof(transicion_t));

    ESP_LOGI(TAG, "HOLA1");
    // Registro eventos.
    err = esp_event_handler_instance_register(
        ESP_EVENT_ANY_BASE, 
        ESP_EVENT_ANY_ID, 
        event_handler, 
        NULL, 
        NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_event_handler_instance_register: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "HOLA2");
    // Iniciación MQTT.
    err = mqtt_api_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en mqtt_api_init: %s", esp_err_to_name(err));
        return;
    }


    TaskHandle_t task_handle;
    xTaskCreate(main_task, "Main task", 2048, NULL, 5, &task_handle);

}
