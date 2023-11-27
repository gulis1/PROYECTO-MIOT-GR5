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
#include "provision.h"
#include "sensores.h"

#include <esp_sntp.h>
#include <esp_netif.h>

#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"

/// HORA
void Get_current_date_time(char *date_time)
{
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Set timezone to Indian Standard Time ///cambiar a madrid
    setenv("TZ", "UTC-05:30", 1);
    tzset();
    localtime_r(&now, &timeinfo);

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("HORA", "The current date/time in Delhi is: %s", strftime_buf);
    strcpy(date_time, strftime_buf);
}

// LONG
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI("SNTP", "Notification of a time synchronization event");
}

void initialize_sntp(void)
{
    ESP_LOGI("SNTP", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    // #ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    // sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    // #endif
    sntp_init();
}

void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

void Set_SystemTime_SNTP(){
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI("SNTP", "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
}

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

    // esto se puede comentar para el merge
    void prov_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {

        ESP_LOGI("PROV_HANDLER", "Evento de provisionamiento recibido.");

        transicion_t trans;
        switch (event_id)
        {

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
    // este es el handler de los valores
    void sensores_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {

        ESP_LOGI("SENSORES_HANDLER", "EVENTO DE DATOS ENVIADOS RECIBIDO"); // Evento de provisionamiento recibido.

        transicion_t trans;
        switch (event_id)
        {

        case SENSORES_ENVIAN_DATO:
            trans.tipo = TRANS_SENSORIZACION;
            xQueueSend(fsm_queue, &trans, portMAX_DELAY);
            break;

        default:
            ESP_LOGI("SENSORES_HANDLER", "Evento desconocido.");
        }
    }
