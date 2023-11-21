#include "SGP30.h"
#include <sensores.h>
#include "si7021.h" //creo que se puede borrar 
#include <i2c_config.h>
#include <driver/i2c.h>
#include <esp_event.h>
#include "esp_timer.h"

const static char* TAG = "Lectura de sensores";

static esp_timer_handle_t periodic_timer;
static esp_event_loop_handle_t sensores_event_handle=NULL;

//Declaramos la familia de eventos
ESP_EVENT_DEFINE_BASE(SENSORES_EVENT) ;


static void lectura_sensores_callback(){


    ESP_LOGI(TAG, "SGP30 main task initializing...");
    float temp;
    sgp30_dev_t main_sgp30_sensor;
    //i2c_master_init_sgp30();
    i2c_master_driver_initialize();
    sgp30_init(&main_sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    // SGP30 needs to be read every 1s and sends TVOC = 400 14 times when initializing //componente
    for (int i = 0; i < 14; i++) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sgp30_IAQ_measure(&main_sgp30_sensor);
        ESP_LOGI(TAG, "SGP30 Calibrating... TVOC: %d,  eCO2: %d",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
    }

    // Read initial baselines 
    uint16_t eco2_baseline, tvoc_baseline;
    sgp30_get_IAQ_baseline(&main_sgp30_sensor, &eco2_baseline, &tvoc_baseline);
    ESP_LOGI(TAG, "BASELINES - TVOC: %d,  eCO2: %d",  tvoc_baseline, eco2_baseline);


    ESP_LOGI(TAG, "Lecturas ejecutando...");
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        sgp30_IAQ_measure(&main_sgp30_sensor);

        readTemperature(0, &temp);
        ESP_LOGI(TAG, "TVOC: %d,  eCO2: %d y la temperatura: %.3f",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2, temp);
        ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, SENSORES_ENVIAN_DATO, &temp, sizeof(temp), portMAX_DELAY)); //¿envio de datos de los demas? envia un evento cada segundo con el dato
            }
    }
    

esp_err_t sensores_init(void *sensores_handler){
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lectura_sensores_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "sensores_callback"
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    esp_err_t err;
    err = esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,sensores_event_handle,NULL);
        if (err!=ESP_OK){
            ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        }

        return ESP_OK;
    }

void sensores_start(){
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, CONFIG_PERIODO_TEMP * 1000000));
}

void monitor_stop(){    
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}

// ESP_LOGI(TAG, "HOLA3"); Para el main
//     //Iniciacion sensores
//     sensores_init();
//     sensores_start();