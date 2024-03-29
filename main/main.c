#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_wifi.h>

#include "main.h"
#include "wifi.h"
#include "provision.h"
#include "sensores.h"
#include "sntp_client.h"
#include "power_mngr.h"
#include "thingsboard.h"
#include "bluetooth.h"
#include "boton.h"

const static char* TAG = "main.c";


void main_task() {

    estado_t estado_actual = ESTADO_SIN_PROVISION;
    if (init_provision(prov_handler) != ESP_OK) {
        ESP_LOGE(TAG, "Provisonment failed.");
        return;
    }

    while (true) {
        
        transicion_t transicion;
        if (xQueueReceive(fsm_queue, &transicion, portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Error en xQueueReceive.");
            continue;
        }

        if (transicion.tipo == TRANS_ERASE_FLASH) {
            trans_estado_to_erase(transicion);
            esp_restart();
        }
      
        switch (estado_actual) {

            case ESTADO_SIN_PROVISION:
                estado_actual = trans_estado_inicial(transicion);
                break;

            case ESTADO_PROVISIONADO:
                estado_actual = trans_estado_provisionado(transicion);
                break;
            
            case ESTADO_CALIBRADO:
                estado_actual = trans_estado_calibrado(transicion);
                break;

            case ESTADO_CONECTADO:
                estado_actual = trans_estado_conectado(transicion);
                break;
        
            case ESTADO_HORA_CONFIGURADA:
                estado_actual = trans_estado_hora_configurada(transicion);
                break;

            case ESTADO_THINGSBOARD_READY:
                estado_actual = trans_estado_thingsboard_ready(transicion);
                break;

            default:
                ESP_LOGE(TAG, "Estado desconocido: %d.", estado_actual);
        }    
    }
}


void app_main(void) {

    esp_err_t err;
    ESP_LOGI(TAG, "Starting firmware version %s", CONFIG_APP_PROJECT_VER);

    // Iniciación flash.
    err = nvs_flash_init();
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

    // Iniciación boton para el erase_flash
    err = boton_init(boton_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en boton_init: %s", esp_err_to_name(err));
        return;
    }

    err = wifi_init_sta(wifi_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en wifi: %s", esp_err_to_name(err));
        return;
    }

    // Creación de la cola.
    fsm_queue = xQueueCreate(16, sizeof(transicion_t));

    err = sensores_init(sensores_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en sensores_init: %s", esp_err_to_name(err));
        return;
    }

    err = thingsboard_init(thingsboard_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en thingsboard_init: %s", esp_err_to_name(err));
        return;
    }
  
    err = tyme_sync_init(hora_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en sntp_init_hora: %s", esp_err_to_name(err));
        return;
    }

    err = power_manager_init(power_manager_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en iniciar power_manager %s", esp_err_to_name(err));
        return;
    }

    TaskHandle_t task_handle;
    xTaskCreate(main_task, "Main task", 4096, NULL, 5, &task_handle);
}
