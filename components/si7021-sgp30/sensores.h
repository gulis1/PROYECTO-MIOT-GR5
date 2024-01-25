#ifndef SENSORES 
#define SENSORES
 
#include <stdio.h>
#include <string.h>

#include <esp_types.h>
#include <sdkconfig.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_err.h>

#include <esp_event.h>

enum{
    CALIBRACION_REALIZADA,
    SENSORES_ENVIAN_DATO
};

ESP_EVENT_DECLARE_BASE(SENSORES_EVENT);

typedef struct {
    uint16_t TVOC_dato;
    uint16_t CO2_dato;
    float temp_dato;
    float hum_dato; 
} data_sensores_t;

esp_err_t sensores_init(void *sensores_handler);
esp_err_t start_calibracion();
esp_err_t sensores_start();
esp_err_t sensores_stop();
esp_err_t sensores_set_periodo(int p);
int sensores_get_periodo();


#endif