
#ifndef CONFIGURACION_HORA
#define CONFIGURACION_HORA

#include <esp_event.h>
#include <esp_log.h>

enum {
    HORA_CONFIGURADA
};

ESP_EVENT_DECLARE_BASE(HORA_CONFIG_EVENT);

esp_err_t sntp_init_hora(void *hora_handler);
esp_err_t start_time_sync();
esp_err_t obtain_time(void);

#endif

