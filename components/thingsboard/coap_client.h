#ifndef COAP_CLIENT
#define COAP_CLIENT

#include <esp_err.h>
#include <coap3/coap.h>

esp_err_t coap_client_init(coap_response_handler_t message_handler, char *cert);
esp_err_t coap_client_start();
esp_err_t coap_client_stop();
esp_err_t coap_set_device_token(char *device_token, unsigned char *attributes_token, unsigned char *rpc_token);
esp_err_t coap_client_telemetry_post(char *content);
esp_err_t coap_client_provision_post(char *content, uint8_t *pdu_token);
esp_err_t coap_client_fw_download(uint8_t *pdu_token, int chunk, int chunk_size);
esp_err_t coap_client_attributes_post(char *content);
esp_err_t coap_client_send_rpc_response(int id, char *payload);
#endif