#include <esp_event.h>

enum {
    HORA_CONFIGURADA
};

ESP_EVENT_DECLARE_BASE(HORA_CONFIG_EVENT);

esp_err_t tyme_sync_init(void *hora_handler);
esp_err_t time_sync_start();