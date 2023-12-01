/*  Fichero de handlers.

    En este fichero van los handlers que tratarán los
    eventos de los distintos componentes del proyecto.

    En estas funciones no debe ir nada de lógica, solo
    se recibirá el evento y se pondrá en la cola un
    valor de tipo "transicion_t", cuyo campo "tipo" se
    establecerá en función del tipo de evento.

    La estructura "transicion_t" contiene un campo "dato"
    que puede usarse para poner en la cola información
    adicional sobre la transición (por ejemplo, el valor
    de la lectura de un sensor).
*/

#include <esp_event.h>
#include <esp_log.h>
#include <mqtt_client.h>

#include "main.h"
#include "wifi.h"
#include "provision.h"
#include "sensores.h"

#include <esp_sntp.h>
#include <esp_netif.h>

#include "esp_check.h"


#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"

#include "esp_sntp.h"
#include "configuracion_hora.h"
#include "sueno_profundo.h"


////////////////////
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_timer.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <i2c_config.h>

#include "esp_pm.h"


#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"

#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include "esp_timer.h"
///////////////////

//static const int DEEP_SLEEP_PERIOD_USEC = 20 * 1000000;

    // Cola de transiciones para la máquina de estados.
    QueueHandle_t fsm_queue;

void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    ESP_LOGI("MQTT_HANDLER", "Evento de MQTT recibido.");

    transicion_t trans;
    switch (event_id)
    {

    case MQTT_EVENT_CONNECTED:
        trans.tipo = TRANS_MQTT_CONNECTED;
        xQueueSend(fsm_queue, &trans, portMAX_DELAY);
        break;

    case MQTT_EVENT_DISCONNECTED:
        trans.tipo = TRANS_MQTT_DISCONNECTED;
        xQueueSend(fsm_queue, &trans, portMAX_DELAY);
        break;
    }
}

void wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const char *TAG;
    transicion_t trans;
    if (event_base == WIFI_EVENT) {
        TAG = "WIFI_HANDLER";
        switch (event_id) {

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "IP ACQUIRED\n");
                break;

            default:
                ESP_LOGE("WIFI_HANDLER", "Evento desconocido.");
            }
    } 
    
    else if (event_base == IP_EVENT) {
        TAG = "IP_HANDLER";
        switch (event_id) {
            
            case IP_EVENT_STA_GOT_IP:
                trans.tipo=TRANS_WIFI_READY;
                xQueueSend(fsm_queue, &trans, portMAX_DELAY);
                ESP_LOGI(TAG, "IP ACQUIRED\n");
                break;

            default:
                ESP_LOGE("IP_HANDLER", "Evento desconocido.");
        }
    }

}

void prov_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    ESP_LOGI("PROV_HANDLER", "Evento de provisionamiento recibido.");

    transicion_t trans;
    switch (event_id) {

        case PROV_DONE:

            prov_info_t *provinfo = *((prov_info_t **)event_data);
            trans.tipo = TRANS_PROVISION;
            trans.dato = provinfo;

            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;

        case PROV_ERROR:
            // TODO
            break;

        default:
            ESP_LOGE("PROV_HANDLER", "Evento desconocido.");
    }
}


/////////////////////////////////////////////
// este es el handler de los sensores
void sensores_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    transicion_t trans;
    switch (event_id) {
        case SENSORES_ENVIAN_DATO:
            data_sensores_t *info_data_sensores_a_pasar = *((data_sensores_t**)event_data);
            trans.tipo = TRANS_LECTURA_SENSORES;
            trans.dato = info_data_sensores_a_pasar;
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;

        case CALIBRACION_REALIZADA:
            trans.tipo = TRANS_CALIBRACION_REALIZADA;
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;
        case PASAR_A_DORMIR_2:
            trans.tipo = TRANS_DORMIR;
            ESP_LOGI("SLEEP_HANLDER", "TRANSCICION PARA DORMIR_2");
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;

        default:
            ESP_LOGI("SENSORES_HANDLER", "Evento desconocido.");
    }
}


//Este es el handler de la Hora
void hora_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

        transicion_t trans;
        switch (event_id) {
        case HORA_CONFIGURADA:
            trans.tipo = TRANS_SINCRONIZAR;
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;
        default:
            ESP_LOGI("HORA_HANDLER", "Evento desconocido.");

    }
}


//este es el halder del deep_sleep
void sleep_timer_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    transicion_t trans;
        switch (event_id) {
        case PASAR_A_DORMIR:
            trans.tipo = TRANS_DORMIR;
            ESP_LOGI("SLEEP_HANLDER", "TRANSCICION PARA DORMIR");
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;
        default:
            ESP_LOGI("HORA_HANDLER", "Evento desconocido.");
    }
}



