#include <stdio.h>
#include <time.h>
#include "SGP30.h"
#include "sensores.h"
#include "si7021.h"
#include <driver/i2c.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <nvs_flash.h>

const static char* TAG = "Lectura de sensores";

static int32_t PERIODO_TEMP;

static esp_timer_handle_t periodic_timer;
static sgp30_dev_t main_sgp30_sensor;


// crear la estatica de la estructura
static data_sensores_t DATA_SENSORES;

//Declaramos la familia de eventos
ESP_EVENT_DEFINE_BASE(SENSORES_EVENT) ;


static void lectura_sensores_callback(){

    float temp;
    float hum;
    sgp30_IAQ_measure(&main_sgp30_sensor);
    readTemperature(0, &temp);
    readHumidity(0,&hum);
    // ESP_LOGI(TAG, "TVOC: %d,  eCO2: %d y la temperatura: %.3f",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2, temp);

    // Estructuracion de los datos para enviar 
    DATA_SENSORES.CO2_dato = main_sgp30_sensor.eCO2;
    DATA_SENSORES.TVOC_dato = main_sgp30_sensor.TVOC;
    DATA_SENSORES.temp_dato = temp;
    DATA_SENSORES.hum_dato = hum;

    void *dato_sensores = &DATA_SENSORES;

    //Envio post
    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, SENSORES_ENVIAN_DATO, &dato_sensores, sizeof(dato_sensores), portMAX_DELAY)); //se envia a la cola?? recordad hacer el free de json  en la transicion cJSON_Delete(root); parece no hizo falta porque el json lo estruturamos de forma manual
}

esp_err_t sensores_init(void *sensores_handler) {

    esp_err_t err;

    ESP_LOGI(TAG, "SGP30 main task initializing...");

    err = i2c_master_driver_initialize();
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error i2c_master_driver_initialize(): %s",esp_err_to_name(err));
        return err;
    }
    
    sgp30_init(&main_sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    // Buscar periodo en la NVS.
    nvs_handle handle_nvs;
    err = nvs_open("nvs", NVS_READONLY, &handle_nvs);
    if (err != ESP_OK || nvs_get_i32(handle_nvs, "periodo_sensor", &PERIODO_TEMP) != ESP_OK)
        PERIODO_TEMP = CONFIG_PERIODO_TEMP;
    
    nvs_close(handle_nvs);

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

    for (int i = 0; i < 15; i++) { 
        sgp30_IAQ_measure(&main_sgp30_sensor);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "SGP30 Calibrando... TVOC: %d,  eCO2: %d",  main_sgp30_sensor.TVOC, main_sgp30_sensor.eCO2);
    }

    ESP_ERROR_CHECK(esp_event_post(SENSORES_EVENT, CALIBRACION_REALIZADA, NULL, 0, portMAX_DELAY));

    vTaskDelete(NULL);
}

esp_err_t start_calibracion(){
     // SGP30 needs to be read every 1s and sends TVOC = 400 14 times when initializing //componente task calibaracion
    xTaskCreate(calibracion, "Tarea calibracion", 4096, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t sensores_start() {

    if (PERIODO_TEMP > 0) {
        esp_err_t err = esp_timer_start_periodic(periodic_timer, PERIODO_TEMP * 1000000);
        return err == ESP_ERR_INVALID_STATE ? ESP_OK : err;
    }
    
    else return ESP_OK;
}

esp_err_t sensores_stop(){    
    esp_err_t err = esp_timer_stop(periodic_timer);
    return err == ESP_ERR_INVALID_STATE ? ESP_OK : err;
}

esp_err_t sensores_set_periodo(int p) {

    sensores_stop();
    PERIODO_TEMP = p;
    if (PERIODO_TEMP > 0)
        sensores_start();   

    nvs_handle handle_nvs;
    nvs_open("nvs", NVS_READWRITE, &handle_nvs);
    nvs_set_i32(handle_nvs, "periodo_sensor", (int32_t) p);
    nvs_close(handle_nvs);  

    return ESP_OK;  
}

int sensores_get_periodo() {
    return PERIODO_TEMP;
}
