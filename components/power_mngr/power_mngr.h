#ifndef SUENO_PROFUNDO
#define SUENO_PROFUNDO
enum {
     PASAR_A_DORMIR
};

ESP_EVENT_DECLARE_BASE(DEEP_SLEEP_EVENT);


esp_err_t power_manager_init(void *hora_handler);
esp_err_t power_manager_start();
void deep_sleep();
#endif
