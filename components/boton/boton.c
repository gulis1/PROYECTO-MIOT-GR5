#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_event.h>

#include "boton.h"

ESP_EVENT_DEFINE_BASE(BOTON_EVENT);

#define BOTON_GPIO 0

const static char* TAG = "Boton Erase Flash";

void event_erase_flash(void){
    
    //event post
    ESP_ERROR_CHECK(esp_event_post(BOTON_EVENT, BOTON_PRESIONADO, NULL, sizeof(NULL), portMAX_DELAY));

}


esp_err_t erase_flash(){
        esp_err_t err;
        // Borra la región de nvs
        err = nvs_flash_erase();
        if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en eliminar flash%s archivo %s y linea %d", esp_err_to_name(err),__FILE__, __LINE__);
        return err;
        }

    return ESP_OK;

}

esp_err_t boton_init(void *boton_handler)
{

    esp_err_t err;
    //declaracion de configuracion del GPIO
     gpio_config_t boton_config = {
        .pin_bit_mask = (1ULL<<BOTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pull_up_en = 1,
    };
    gpio_config(&boton_config);

    // Configurar interrupción para el botón
    err=gpio_install_isr_service(0);
    if (err!= ESP_OK){
        ESP_LOGE(TAG, "Error en install _isr_service %s archivo %s y linea %d", esp_err_to_name(err),__FILE__,__LINE__);
        return err;
    }
    
    err=gpio_isr_handler_add(BOTON_GPIO, event_erase_flash, (void*) BOTON_GPIO);
    if (err!= ESP_OK){
        ESP_LOGE(TAG, "Error en install _isr_service %s archivo %s y linea %d", esp_err_to_name(err),__FILE__,__LINE__);
        return err;
    }

    //registro
    err = esp_event_handler_register(BOTON_EVENT, ESP_EVENT_ANY_ID, boton_handler, NULL);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    return ESP_OK;

}