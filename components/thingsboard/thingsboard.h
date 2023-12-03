#ifndef THINGSBOARD
#define THINGSBOARD

enum {
    THINGSBOARD_EVENT_READY,
    THINGSBOARD_EVENT_UNAVAILABLE,
    THINGSBOARD_PROVISIONED
};

ESP_EVENT_DECLARE_BASE(THINGSBOARD_EVENT);

esp_err_t thingsboard_init(void *eventhandler);
esp_err_t thingsboard_start();
esp_err_t thingsboard_stop();
esp_err_t thingsboard_telemetry_send();

#endif