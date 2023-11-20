#include "SGP30.h"
#include <sensores.h>
#include "si7021.h" //creo que se puede borrar 
#include <i2c_config.h>
#include <driver/i2c.h>
#include <esp_event.h>

const static char* TAG = "Lectura de sensores";


//mirar para declarar el define eventro etc
ESP_EVENT_DEFINE_BASE(SENSORES_EVENT) ;

//posteamos evento de la familia SENSORES_EVENT dato enviado
static int sensores_posteo_evento(){
    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, SENSORES_ENVIAN_DATO,NULL,0,portMAX_DELAY));
    return 0;
}



void lectura_sensores(){


    ESP_LOGI(TAG, "SGP30 main task initializing...");
    float temp;
    sgp30_dev_t main_sgp30_sensor;
    //i2c_master_init_sgp30();
    i2c_master_driver_initialize();
    sgp30_init(&main_sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    // SGP30 needs to be read every 1s and sends TVOC = 400 14 times when initializing
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
        //ESP_LOGI(TAG, "Lectura temperatura: %.3f", temp);
        for (int i = 0; i < 14; i++){
            //enviar evento datos enviados
        }
    }
}