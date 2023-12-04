#ifndef COAP_CLIENT
#define COAP_CLIENT

#include <esp_err.h>
#include <coap3/coap.h>

esp_err_t coap_client_init(coap_response_handler_t message_handler, char *device_token);
esp_err_t coap_client_telemetry_send(char *content);
esp_err_t coap_client_provision_send(char *content);

#endif