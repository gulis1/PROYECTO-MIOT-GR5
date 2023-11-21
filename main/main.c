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


void main_task() {

    estado_t estado_actual = ESTADO_SIN_PROVISION;
    // TODO: inciar provisionamiento.

    while (true) {

        transicion_t transicion;
        if (xQueueReceive(fsm_queue, &transicion, portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Error en xQueueReceive.");
            continue;
        }

        switch (estado_actual) {

            case ESTADO_SIN_PROVISION:
                trans_estado_inicial(transicion);
                break;
                
            default:
                ESP_LOGE(TAG, "Estado desconocido: %d.", estado_actual);
        }
    }
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
    fsm_queue = xQueueCreate(16, sizeof(transicion_t));

    // Iniciación MQTT. Se le pasa el handler de los eventos
    // MQTT para que se registre. 
    err = mqtt_api_init(mqtt_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en mqtt_api_init: %s", esp_err_to_name(err));
        return;
    }

    TaskHandle_t task_handle;
    xTaskCreate(main_task, "Main task", 2048, NULL, 5, &task_handle);
}
