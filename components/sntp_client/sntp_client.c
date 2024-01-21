#include <time.h>
#include <sys/time.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif_sntp.h>

#include "sntp_client.h"

static const char *TAG = "Configuracion_Hora";

ESP_EVENT_DEFINE_BASE(HORA_CONFIG_EVENT);

static esp_err_t obtain_time(void) {
    
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.windows.com");

    esp_netif_sntp_init(&config);

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Esperando el sistema para configurar hora... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    esp_netif_sntp_deinit();

    return ESP_OK;
}

static void sincronizacion_hora() {
    
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "El tiempo no este configurado aun. Obteniendo el tiempo protocol SNTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    // set timezona en madrid
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1); //https://gist.github.com/alwynallan/24d96091655391107939
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "La fecha y hora actual de Madrid es: %s", strftime_buf);

    //event post
    ESP_ERROR_CHECK(esp_event_post(HORA_CONFIG_EVENT, HORA_CONFIGURADA, NULL, sizeof(NULL), portMAX_DELAY));

    vTaskDelete(NULL);
}

esp_err_t tyme_sync_init(void *hora_handler) {

    esp_err_t err;
    err = esp_event_handler_register(HORA_CONFIG_EVENT, ESP_EVENT_ANY_ID, hora_handler, NULL);
    if (err!=ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t time_sync_start() {
    xTaskCreate(sincronizacion_hora,"Tarea sincronizacion", 4096, NULL, 5, NULL);
    return ESP_OK;
}



