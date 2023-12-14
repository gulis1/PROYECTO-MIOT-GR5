#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sdkconfig.h>
#include <soc/soc_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/rtc_io.h>
#include <soc/rtc.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_pm.h>



#include "main.h"
#include "esp_check.h"
#include "esp_sleep.h"
#include "sueno_profundo.h"
#include "configuracion_hora.h"

//#define TIMER_WAKEUP_TIME_US    (2 * 1000 * 1000) //TODOANGEL: se puede poner en menuconfig seg*1000*1000 2=20 segundos
#define HORA_INICIO  CONFIG_HORA_INICIO
#define MINUTOS_INICIO CONFIG_MINUTO_INICIO
#define SEGUNDOS_INICIO CONFIG_SEGUNDO_INICIO


#define HORA_FINAL  CONFIG_HORA_FINAL
#define MINUTO_FINAL CONFIG_MINUTO_FINAL
#define SEGUNDO_FINAL CONFIG_SEGUNDO_FINAL

ESP_EVENT_DEFINE_BASE(DEEP_SLEEP_EVENT);
static const char *TAG = "timer_wakeup";
//char strftime_buf[64];

time_t ahora;
struct tm *miTiempo;


time_t momento_final;
struct tm *tiempo_final;



int traspaso=0;
int segundos_para_dormir;
bool tipo_cambio_hora;
int segundos_actuales;
// int segundos_finales;
// int segundos_iniciales;
uint64_t momento_dormido;
uint64_t segundos_para_despertar;
int diferencia;
char hora_actual[80];

int mitiempo_en_segundos;
int mitiempo_en_minutos;
int mitiempo_en_horas;

int segundos_finales= HORA_FINAL*3600 + MINUTO_FINAL*60+SEGUNDO_FINAL;
int segundos_iniciales= HORA_INICIO*3600 + MINUTOS_INICIO*60 + SEGUNDOS_INICIO;

static esp_timer_handle_t periodic_timer_2;


void hora(void){
    
    time(&ahora); // Obtener el tiempo actual

    miTiempo = localtime(&ahora); // Convertir a struct tm
    mitiempo_en_segundos=miTiempo->tm_sec;
    mitiempo_en_minutos=miTiempo->tm_min;
    mitiempo_en_horas=miTiempo->tm_hour;
    
    

    // ESP_LOGI("mirando la hora", "hora %d",mitiempo_en_horas);
    // ESP_LOGI("mirando los minutos", "minutos %d",mitiempo_en_minutos);
    // ESP_LOGI("mirando los segundo", "segundos %d",mitiempo_en_segundos);

    strftime(hora_actual, 80, "%Y-%m-%d %H:%M:%S", miTiempo); // Formatear la fecha y hora
    strcpy(strftime_buf, hora_actual);
    ESP_LOGI("Status_hora","La hora es %s",strftime_buf);

    
    segundos_actuales = mitiempo_en_horas*3600 + mitiempo_en_minutos*60+mitiempo_en_segundos;
    momento_dormido=segundos_actuales;

    // CASE FINAL>INICIAL OJO
    if (traspaso!=1){ //Esto permite que al momento de encender reiniciar como la hora no esta sincronizada la sincronice nuevamente.
        ESP_LOGI("Inicializacion en proceso","configurando");
    } else{
        if (segundos_actuales>segundos_finales){//((mitiempo_en_horas>HORA_FINAL || mitiempo_en_minutos>MINUTO_FINAL|| mitiempo_en_segundos> SEGUNDO_FINAL)){
            printf("hola deberia estar dormido el tiempo es mayor");
            ESP_LOGI("transicion","transicion numero %d",traspaso);
            //momento_dormido=segundos_actuales;
            segundos_para_despertar = (86400-momento_dormido)+((HORA_INICIO)*3600)+(MINUTOS_INICIO*60)+(SEGUNDOS_INICIO); //mass minutos y segundos
            ESP_LOGI(TAG, "Entrando en deep sleep. quedan %llu segundos para despertar",segundos_para_despertar);
            esp_sleep_enable_timer_wakeup(segundos_para_despertar * 1000 * 1000);
            deep_sleep();
        }else if (segundos_actuales<segundos_iniciales){//((mitiempo_en_horas<HORA_INICIO|| mitiempo_en_minutos<MINUTOS_INICIO|| mitiempo_en_segundos<SEGUNDOS_INICIO))
            printf("hola deberia estar dormido el tiempo es menor");
            ESP_LOGI("transicion","transicion numero %d",traspaso);
            //momento_dormido=segundos_actuales;
            segundos_para_despertar = segundos_iniciales-segundos_actuales;//(((HORA_INICIO-mitiempo_en_horas)*3600) + ((MINUTOS_INICIO-mitiempo_en_minutos)*60) + (MINUTOS_INICIO-mitiempo_en_segundos))-86400; //mass minutos y segundos
            ESP_LOGI(TAG, "Entrando en deep sleep. quedan %llu segundos para despertar",segundos_para_despertar);
            esp_sleep_enable_timer_wakeup(segundos_para_despertar * 1000 * 1000);
            deep_sleep();
        }
        
        //TODO se puede poner deepsleep:
    }
    vTaskDelay(500 / portTICK_PERIOD_MS); //bloquea la tarea por menos de un segundo para intentar enentrear en suño se puede que juso antes del tiempo de mientreo, sacando un porcentaje o similar 

} 

esp_err_t init_register_timer_wakeup(void *hora_handler)
{
    esp_err_t err;

    ESP_LOGI(TAG, "init_register_timer_wakeup");

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = hora,
        /* name is optional, but may help identify the timer when debugging */
        .name = "reloj"
    };

    err = esp_timer_create(&periodic_timer_args, &periodic_timer_2);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_timer_create: %s",esp_err_to_name(err));
        return err;
    }
    
    err = esp_event_handler_register(DEEP_SLEEP_EVENT, ESP_EVENT_ANY_ID, hora_handler, NULL);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void entrar_deep_sleep(){
    const char *TAG = "deep_sleep";
            printf("hola deberia estar dormido");
            ESP_LOGI(TAG, "Entrando en deep sleep.");
            esp_deep_sleep_start();
            // envent post
            ESP_ERROR_CHECK(esp_event_post(DEEP_SLEEP_EVENT,PASAR_A_DORMIR, NULL, sizeof(NULL), portMAX_DELAY));
    vTaskDelete(NULL);
}

esp_err_t deep_sleep(){
     xTaskCreate(entrar_deep_sleep, "entrar_deep_sleep task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t comienza_reloj(){
    //  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_2, 1 * 1000000));
    return ESP_OK;
}


//// configuracion del power_manangment

esp_err_t power_manager_init(){

    //configuracion parametros
    esp_pm_config_t power_config={
        .max_freq_mhz=240,
        .min_freq_mhz=80,
        .light_sleep_enable=true
    };

    esp_err_t ret = esp_pm_configure(&power_config);
    if (ret != ESP_OK) {
        ESP_LOGE("Power Manager", "init power manager falló: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI("Power Manager", "Init correcto");
    return ESP_OK;
}

//TODOANGEL:PUEDEDARSE EL CASO DE QUE QUERAMOS QUE TRABASE DE MADRUGADA YO CREO QUE NO 
//TODOANGEL: Puede pasarse el esp_err_t err a global


