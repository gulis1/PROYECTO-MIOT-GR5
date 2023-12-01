#ifndef COAP_SERVER
#define COAP_SERVER

#include <esp_err.h>

esp_err_t coap_server_init();
esp_err_t coap_server_start();
esp_err_t coap_server_stop();

#endif