#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include <esp_event.h>
#include "main.h"

///timer wakeup

#include "esp_check.h"
#include "esp_sleep.h"
#include "sueno_profundo.h"
#include "configuracion_hora.h"

#define TIMER_WAKEUP_TIME_US    (2 * 1000 * 1000) //TODOANGEL: se puede poner en menuconfig seg*1000*1000 2=20 segundos


ESP_EVENT_DEFINE_BASE(DEEP_SLEEP_EVENT);


static const char *TAG = "timer_wakeup";
//char strftime_buf[64];


time_t ahora;
struct tm *miTiempo;
struct tm *mitiempo_en_segundos;
struct tm *mitiempo_en_minutos;
struct tm *mitiempo_en_horas;

//char Current_Date_Time[100];

//auxiliar hora
void hora(void){
    
    time(&ahora); // Obtener el tiempo actual

    miTiempo = localtime(&ahora); // Convertir a struct tm
    
    mitiempo_en_segundos=miTiempo->tm_sec;
    mitiempo_en_minutos=miTiempo->tm_min;
    mitiempo_en_horas=miTiempo->tm_hour;

    strftime(hora_actual, 80, "%Y-%m-%d %H:%M:%S", miTiempo); // Formatear la fecha y hora
}

esp_err_t init_register_timer_wakeup(void *hora_handler)
{
    //uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
    ESP_LOGI(TAG, "timer wakeup source is ready");
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup(20 * 1000 * 1000), TAG, "Configure timer as wakeup source failed");
    
    esp_err_t err;
    err = esp_event_handler_register(DEEP_SLEEP_EVENT, ESP_EVENT_ANY_ID, hora_handler, NULL);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    if(mitiempo_en_horas>=3 && mitiempo_en_horas<8){
        printf("hola deberia estar dormido");
        ESP_LOGI(TAG, "Entrando en deep sleep.");
        esp_deep_sleep_start();
        // envent post
        ESP_ERROR_CHECK(esp_event_post(DEEP_SLEEP_EVENT,PASAR_A_DORMIR, NULL, sizeof(NULL), portMAX_DELAY));   
        }
    return ESP_OK;
}





void entrar_deep_sleep(){
    const char *TAG = "deep_sleep";

    
    if(mitiempo_en_horas>=0 && mitiempo_en_horas<8){
            printf("hola deberia estar dormido");
            ESP_LOGI(TAG, "Entrando en deep sleep.");
            esp_deep_sleep_start();
            // envent post
            ESP_ERROR_CHECK(esp_event_post(DEEP_SLEEP_EVENT,PASAR_A_DORMIR, NULL, sizeof(NULL), portMAX_DELAY));   
        }
    vTaskDelete(NULL);
}

esp_err_t deep_sleep(){
     xTaskCreate(entrar_deep_sleep, "entrar_deep_sleep task", 4096, NULL, 5, NULL);
    return ESP_OK;
}



    


