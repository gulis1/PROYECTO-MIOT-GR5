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


#endif