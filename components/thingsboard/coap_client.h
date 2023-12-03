#ifndef COAP_SERVER
#define COAP_SERVER

#include <esp_err.h>

esp_err_t coap_client_init();
esp_err_t coap_client_telemetry_send(char *content);

#endif