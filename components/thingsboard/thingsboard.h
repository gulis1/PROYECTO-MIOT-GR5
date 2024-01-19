#ifndef THINGSBOARD
#define THINGSBOARD

enum {
    THINGSBOARD_EVENT_READY,
    THINGSBOARD_EVENT_UNAVAILABLE,
    THINGSBOARD_PROVISIONED,
    THINGSBOARD_FW_UPDATE_READY
};

ESP_EVENT_DECLARE_BASE(THINGSBOARD_EVENT);

esp_err_t thingsboard_init(void *eventhandler);
esp_err_t thingsboard_start();
esp_err_t thingsboard_stop();
esp_err_t thingsboard_attributes_send(char *content);
esp_err_t thingsboard_telemetry_send(char *msg);
esp_err_t coap_set_device_token(char *device_token);

#endif