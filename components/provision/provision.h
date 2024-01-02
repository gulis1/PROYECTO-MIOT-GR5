#ifndef PROVISIONAMIENTO
#define PROVISIONAMIENTO

#include "esp_event.h"

typedef struct {
    char* wifi_ssid;
    char* wifi_pass;
} prov_info_t;

enum {
    PROV_DONE,
    PROV_ERROR
};

ESP_EVENT_DECLARE_BASE(PROVISION_EVENT);

esp_err_t init_provision(void *event_handler);
esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data);
void start_provisioning();


#endif