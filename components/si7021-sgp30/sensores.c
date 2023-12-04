#include "SGP30.h"
#include <sensores.h>
#include "si7021.h" //creo que se puede borrar 
#include <i2c_config.h>
#include <driver/i2c.h>
#include <esp_event.h>
#include "esp_timer.h"

const static char* TAG = "Lectura de sensores";

static esp_timer_handle_t periodic_timer;

static sgp30_dev_t main_sgp30_sensor;


// crear la estatica de la estructura
static data_sensores_t DATA_SENSORES;



//Declaramos la familia de eventos
ESP_EVENT_DEFINE_BASE(SENSORES_EVENT) ;


static void lectura_sensores_callback(){

    float temp;
    sgp30_IAQ_measure(&main_sgp30_sensor);
    readTemperature(0, &temp);

    // Estructuracion de los datos para enviar 
    DATA_SENSORES.CO2_dato = main_sgp30_sensor.eCO2;
    DATA_SENSORES.TVOC_dato = main_sgp30_sensor.TVOC;
    DATA_SENSORES.temp_dato = temp;

    void *dato_sensores =&DATA_SENSORES;

    //Envio post
    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, SENSORES_ENVIAN_DATO, &dato_sensores, sizeof(dato_sensores), portMAX_DELAY));
}
    

esp_err_t sensores_init(void *sensores_handler) {

    esp_err_t err;

    ESP_LOGI(TAG, "SGP30 main task initializing...");
    //i2c_master_init_sgp30();

    err = i2c_master_driver_initialize();
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error i2c_master_driver_initialize(): %s",esp_err_to_name(err));
        return err;
    }
    sgp30_init(&main_sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);


    const esp_timer_create_args_t periodic_timer_args = {
        .callback = lectura_sensores_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "sensores_callback"
    };

    err = esp_timer_create(&periodic_timer_args, &periodic_timer);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_timer_create: %s",esp_err_to_name(err));
        return err;
    }

    err = esp_event_handler_register(SENSORES_EVENT, ESP_EVENT_ANY_ID, sensores_handler, NULL);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    //tarea de calibracion 

    return ESP_OK;
}



void calibracion() {

    for (int i = 0; i < 14; i++) { 
        sgp30_IAQ_measure(&main_sgp30_sensor);
        ESP_LOGI(TAG, "SGP30 Calibrating... TVOC: %d,  eCO2: %d",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
    }


    //Envio post
    //ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT,CALIBRACION_REALIZADA, &DATA_SENSORES, sizeof(DATA_SENSORES), portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT,CALIBRACION_REALIZADA, NULL, sizeof(NULL), portMAX_DELAY));
    
    // TODO: guradar valores calibracion en la flash 

    // // Read initial baselines 
    // uint16_t eco2_baseline, tvoc_baseline;
    // sgp30_get_IAQ_baseline(&main_sgp30_sensor, &eco2_baseline, &tvoc_baseline);
    // ESP_LOGI(TAG, "BASELINES - TVOC: %d,  eCO2: %d",  tvoc_baseline, eco2_baseline);

    vTaskDelete(NULL);
}



esp_err_t init_calibracion(){
     // SGP30 needs to be read every 1s and sends TVOC = 400 14 times when initializing //componente task calibaraciom 
    xTaskCreate(calibracion, "task calibracion", 4096, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t sensores_start() {

    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, CONFIG_PERIODO_TEMP * 1000000));
    return ESP_OK;
}

esp_err_t sensores_stop(){    
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    return ESP_OK;
}