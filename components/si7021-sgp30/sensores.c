#include <stdio.h>
#include <time.h>
#include "SGP30.h"
#include <sensores.h>
#include "si7021.h" //creo que se puede borrar 
#include <i2c_config.h>
#include <driver/i2c.h>
#include <esp_event.h>
#include <esp_timer.h>
#include "main.h"
#include "cJSON.h"
//#include "configuracion_hora.h"

const static char* TAG = "Lectura de sensores";

static esp_timer_handle_t periodic_timer;

static sgp30_dev_t main_sgp30_sensor;


// crear la estatica de la estructura
static data_sensores_t DATA_SENSORES;

//char info;


//Declaramos la familia de eventos
ESP_EVENT_DEFINE_BASE(SENSORES_EVENT);

//static int conteo=0;


char* data_sensores_to_json_string(data_sensores_t* info) {

    cJSON *root = cJSON_CreateObject();

     /// la idea es devolver un objeto json 
    cJSON_AddNumberToObject(root,"temperatura",info->temp_dato);
    cJSON_AddNumberToObject(root,"eCO2",info->CO2_dato);
    cJSON_AddNumberToObject(root,"TVOC",info->TVOC_dato);

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string;
}


static void lectura_sensores_callback(){

    float temp;
    sgp30_IAQ_measure(&main_sgp30_sensor);
    readTemperature(0, &temp);
    ESP_LOGI(TAG, "TVOC: %d,  eCO2: %d y la temperatura: %.3f",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2, temp);

    // Estructuracion de los datos para enviar 
    DATA_SENSORES.CO2_dato = main_sgp30_sensor.eCO2;
    DATA_SENSORES.TVOC_dato = main_sgp30_sensor.TVOC;
    DATA_SENSORES.temp_dato = temp;

    void *dato_sensores =&DATA_SENSORES;

    //Envio post
    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, SENSORES_ENVIAN_DATO, &dato_sensores, sizeof(dato_sensores), portMAX_DELAY)); //se envia a la cola?? recordad hacer el free de json  en la transicion cJSON_Delete(root);
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
     // El SGP30 inicializa haciendo un envio de TVOC =400 14 veces 
    xTaskCreate(calibracion, "task calibracion", 1024, NULL, 5, NULL); //4096
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
