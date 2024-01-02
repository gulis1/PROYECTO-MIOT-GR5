#ifndef SUENO_PROFUNDO
#define SUENO_PROFUNDO

#include <esp_event.h>
#include <esp_log.h>


enum {
     PASAR_A_DORMIR
};

ESP_EVENT_DECLARE_BASE(DEEP_SLEEP_EVENT);


esp_err_t init_register_timer_wakeup(void *hora_handler);
esp_err_t deep_sleep();

#endif