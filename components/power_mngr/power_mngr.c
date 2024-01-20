#include <time.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#include <esp_log.h>

#include "power_mngr.h"

ESP_EVENT_DEFINE_BASE(DEEP_SLEEP_EVENT);

static const char *TAG = "Power manager";
static esp_timer_handle_t timer_reloj;

static int segundos_iniciales = CONFIG_HORA_INICIO * 3600 + CONFIG_MINUTO_INICIO * 60;
static int segundos_finales = CONFIG_HORA_FINAL * 3600 + CONFIG_MINUTO_FINAL * 60;

static int hora_actual_segundos() {

    time_t ahora;
    char hora_str[80];

    time(&ahora); 
    struct tm *tiempo = localtime(&ahora);
    strftime(hora_str, 80, "%Y-%m-%d %H:%M:%S", tiempo); 
    ESP_LOGI(TAG,"La hora es %s", hora_str);

    return tiempo->tm_hour * 3600 + tiempo->tm_min * 60 + tiempo->tm_sec;
}

static void callback_reloj(void *arg) {

    int segundos_actuales = hora_actual_segundos();

    // Si el rango está en el mismo día.
    if (segundos_finales > segundos_iniciales && (segundos_actuales <= segundos_iniciales || segundos_actuales >= segundos_finales))
        esp_event_post(DEEP_SLEEP_EVENT, PASAR_A_DORMIR, NULL, 0, portMAX_DELAY);
    
    else if (segundos_iniciales < segundos_finales && segundos_actuales <= segundos_iniciales && segundos_actuales >= segundos_finales)
        esp_event_post(DEEP_SLEEP_EVENT, PASAR_A_DORMIR, NULL, 0, portMAX_DELAY);
} 

esp_err_t power_manager_init(void *hora_handler) {

    esp_err_t err;

    //configuracion parametros
    // esp_pm_config_t power_config = {
    //     .max_freq_mhz=240,
    //     .min_freq_mhz=80,
    //     .light_sleep_enable=true
    // };

    // esp_err_t ret = esp_pm_configure(&power_config);
    // if (ret != ESP_OK) {
    //     ESP_LOGE("Power Manager", "init power manager falló: %s", esp_err_to_name(ret));
    //     return ret;
    // }

    const esp_timer_create_args_t timer_args = {
        .callback = callback_reloj,
        .name = "reloj"
    };

    err = esp_timer_create(&timer_args, &timer_reloj);
    if (err != ESP_OK){
        ESP_LOGE(TAG,"Error en esp_timer_create: %s",esp_err_to_name(err));
        return err;
    }
    
    err = esp_event_handler_register(DEEP_SLEEP_EVENT, ESP_EVENT_ANY_ID, hora_handler, NULL);
    if (err != ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t power_manager_start() {

    esp_err_t err = esp_timer_start_periodic(timer_reloj, 60 * 1000000);
    if (err != ESP_OK){
        ESP_LOGE(TAG,"Error en esp_event_handler_register: %s",esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void deep_sleep() {

    int segundos_para_despertar;
    int segundos_actuales = hora_actual_segundos();
    
    if (segundos_iniciales > segundos_actuales)
        segundos_para_despertar = segundos_iniciales - segundos_actuales;
    else 
        segundos_para_despertar = 86400 - segundos_actuales + segundos_iniciales;
 
    esp_sleep_enable_timer_wakeup(segundos_para_despertar * 1000 * 1000);
    esp_deep_sleep_start();
}



