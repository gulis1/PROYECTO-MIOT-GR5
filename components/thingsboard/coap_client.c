/* CoAP client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * WARNING
 * libcoap is not multi-thread safe, so only this thread must make any coap_*()
 * calls.  Any external (to this thread) data transmitted in/out via libcoap
 * therefore has to be passed in/out by xQueue*() via this thread.
 */

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_event.h>
#include <lwip/sockets.h>
#include <coap3/coap.h>
#include "coap_client.h"

#ifdef CONFIG_USE_COAP

const static char *TAG = "CoAP_client";

static char *COAP_SERVER_URI = NULL;
static char *SERVER_CERT = NULL;
static coap_uri_t uri;
static coap_context_t *coap_ctx = NULL;
static coap_session_t *coap_session = NULL;
static coap_optlist_t *optlist_telemetry = NULL;
static coap_optlist_t *optlist_attributes = NULL;
static coap_optlist_t *optlist_firmware = NULL;
static char *DEVICE_TOKEN = NULL;

static void coap_io_callback() {

    while (true) {
        coap_io_process(coap_ctx, CONFIG_COAP_WAIT_MS);
    }
}

static coap_address_t *coap_get_address(coap_uri_t *uri) {

    static coap_address_t dst_addr;
    char *phostname = NULL;
    struct addrinfo hints;
    struct addrinfo *addrres;
    int error;
    char tmpbuf[INET6_ADDRSTRLEN];

    phostname = (char *)calloc(1, uri->host.length + 1);
    if (phostname == NULL) {
        ESP_LOGE(TAG, "calloc failed");
        return NULL;
    }
    memcpy(phostname, uri->host.s, uri->host.length);

    memset ((char *)&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    error = getaddrinfo(phostname, NULL, &hints, &addrres);
    if (error != 0) {
        ESP_LOGE(TAG, "DNS lookup failed for destination address %s. error: %d", phostname, error);
        free(phostname);
        return NULL;
    }
    if (addrres == NULL) {
        ESP_LOGE(TAG, "DNS lookup %s did not return any addresses", phostname);
        free(phostname);
        return NULL;
    }
    free(phostname);
    coap_address_init(&dst_addr);
    switch (addrres->ai_family) {
    case AF_INET:
        memcpy(&dst_addr.addr.sin, addrres->ai_addr, sizeof(dst_addr.addr.sin));
        dst_addr.addr.sin.sin_port        = htons(uri->port);
        inet_ntop(AF_INET, &dst_addr.addr.sin.sin_addr, tmpbuf, sizeof(tmpbuf));
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", tmpbuf);
        break;
    case AF_INET6:
        memcpy(&dst_addr.addr.sin6, addrres->ai_addr, sizeof(dst_addr.addr.sin6));
        dst_addr.addr.sin6.sin6_port        = htons(uri->port);
        inet_ntop(AF_INET6, &dst_addr.addr.sin6.sin6_addr, tmpbuf, sizeof(tmpbuf));
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", tmpbuf);
        break;
    default:
        ESP_LOGE(TAG, "DNS lookup response failed");
        return NULL;
    }
    freeaddrinfo(addrres);

    return &dst_addr;
}

static esp_err_t coap_generate_attributes_optlist()  {

    u_char buf[4];
    char uri_buf[128];
    sprintf(uri_buf, "api/v1/%s/attributes", DEVICE_TOKEN);
    ESP_LOGI(TAG, "COAP attributes URI: %s", uri_buf);

    struct coap_optlist_t *opt_format = coap_new_optlist(COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof (buf), COAP_MEDIATYPE_APPLICATION_JSON), buf);
    if (opt_format == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }
    
    if (coap_insert_optlist(&optlist_attributes, opt_format) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    coap_optlist_t *opt_path = coap_new_optlist(COAP_OPTION_URI_PATH, strlen(uri_buf), (u_char*) uri_buf);
    if (opt_path == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }

    if (coap_insert_optlist(&optlist_attributes, opt_path) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    coap_optlist_t *opt_observe = coap_new_optlist(COAP_OPTION_OBSERVE, coap_encode_var_safe(buf, sizeof (buf), COAP_OBSERVE_ESTABLISH), buf);
    if (opt_observe == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }

    if (coap_insert_optlist(&optlist_attributes, opt_observe) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t coap_generate_telemetry_optlist() {

    u_char buf[4];
    char uri_buf[128];
    sprintf(uri_buf, "api/v1/%s/telemetry", DEVICE_TOKEN);
    ESP_LOGI(TAG, "COAP telemetry URI: %s", uri_buf);

    struct coap_optlist_t *opt_format = coap_new_optlist(COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof (buf), COAP_MEDIATYPE_APPLICATION_JSON), buf);
    if (opt_format == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }
    
    if (coap_insert_optlist(&optlist_telemetry, opt_format) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    coap_optlist_t *opt_path = coap_new_optlist(COAP_OPTION_URI_PATH, strlen(uri_buf), (u_char*) uri_buf);
    if (opt_path == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }

    if (coap_insert_optlist(&optlist_telemetry, opt_path) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t coap_generate_firmware_optlist() {

    char uri_buf[128];
    sprintf(uri_buf, "fw/%s", DEVICE_TOKEN);
    ESP_LOGI(TAG, "COAP fiwmare update URI: %s", uri_buf);

    coap_optlist_t *opt_path = coap_new_optlist(COAP_OPTION_URI_PATH, strlen(uri_buf), (u_char*) uri_buf);
    if (opt_path == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }

    if (coap_insert_optlist(&optlist_firmware, opt_path) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t coap_client_attributes_observe() {

    size_t tokenlength;
    unsigned char token[8];
    coap_pdu_t *request = NULL;

    if (optlist_attributes == NULL) {
        ESP_LOGI(TAG, "optlist_ota is null");
        return ESP_FAIL;
    }

    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_GET, coap_session);
    if (!request) {
        ESP_LOGE(TAG, "Error en coap_new_pdu()");
        return ESP_FAIL;
    }
    
    /* Add in an unique token */
    coap_session_new_token(coap_session, &tokenlength, token);
    if (coap_add_token(request, tokenlength, token) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_token()");
        return ESP_FAIL;
    }
    
    if (coap_add_optlist_pdu(request, &optlist_attributes) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_optlist_pdu()");
        return ESP_FAIL;
    }

    if (coap_send(coap_session, request) == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "Error en coap_send()");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

static esp_err_t coap_client_post(char *content, coap_optlist_t **optlist) {
    
    size_t tokenlength;
    unsigned char token[8];
    coap_pdu_t *request = NULL;

    request = coap_new_pdu(COAP_MESSAGE_NON, COAP_REQUEST_CODE_POST, coap_session);
    if (!request) {
        ESP_LOGE(TAG, "Error en coap_new_pdu()");
        return ESP_FAIL;
    }
    
    /* Add in an unique token */
    coap_session_new_token(coap_session, &tokenlength, token);
    if (coap_add_token(request, tokenlength, token) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_token()");
        return ESP_FAIL;
    }
    
    if (coap_add_optlist_pdu(request, optlist) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_optlist_pdu()");
        return ESP_FAIL;
    }

    if (coap_add_data(request, strlen(content), (unsigned char*) content) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_data_large_request()");
        return ESP_FAIL;
    }

    if (coap_send(coap_session, request) == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "Error en coap_send()");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t coap_client_init(coap_response_handler_t message_handler, char *cert) {

    if (!coap_dtls_is_supported()) {
        ESP_LOGE(TAG, "Coap DTLS not supported");
        return ESP_FAIL;
    }

    SERVER_CERT = cert;

    COAP_SERVER_URI = malloc(256); 
    if (COAP_SERVER_URI == NULL) {
        ESP_LOGE(TAG, "Error en malloc para COAP_SERVER_URI.");
        return ESP_ERR_NO_MEM;
    }
    sprintf(COAP_SERVER_URI, "coaps://%s:5684", CONFIG_THINGSBOARD_URL);

    /* Set up the CoAP context */
    coap_ctx = coap_new_context(NULL);
    if (coap_ctx == NULL) {
        ESP_LOGE(TAG, "Erro en coap_new_context()");
        return ESP_FAIL;
    }

    coap_register_response_handler(coap_ctx, message_handler);
    xTaskCreate(coap_io_callback, "COAP io task", 6144, NULL, 6, NULL);

    return ESP_OK;
}

esp_err_t coap_client_start() {

    if (coap_split_uri((const uint8_t *)COAP_SERVER_URI, strlen(COAP_SERVER_URI), &uri) == -1) {
        ESP_LOGE(TAG, "Error en coap_split_uri()");
        return ESP_FAIL;
    }

    coap_address_t *dst_addr = coap_get_address(&uri);
    if (dst_addr == NULL) {
        ESP_LOGE(TAG, "Error en coap_get_address()");
        return ESP_FAIL;
    }

    coap_set_log_level(COAP_LOG_DEBUG);

    // Create the DTLS session
    coap_dtls_pki_t dtls =  {
        .version = COAP_DTLS_PKI_SETUP_VERSION,
        .verify_peer_cert = 1,        // Verify peer certificate
        .allow_self_signed = 0,
        .allow_expired_certs = 0,     // No expired certificates
        .cert_chain_validation = 1,   // Validate the chain
        .cert_chain_verify_depth = 3,
        .pki_key = {
            .key.pem_buf.ca_cert = (const uint8_t*) SERVER_CERT,
            .key.pem_buf.ca_cert_len = strlen(SERVER_CERT),
            .key_type = COAP_PKI_KEY_PEM_BUF
        }
    };

    coap_session = coap_new_client_session_pki(coap_ctx, NULL, dst_addr, COAP_PROTO_DTLS, &dtls);
    if (coap_session == NULL) {
        ESP_LOGE(TAG, "coap_new_client_session() failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t coap_client_provision_post(char *content, unsigned char *token) {

    size_t tokenlength;
    coap_pdu_t *request = NULL;

    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_POST, coap_session);
    if (!request) {
        ESP_LOGE(TAG, "Error en coap_new_pdu()");
        return ESP_FAIL;
    }
    
    /* Add in an unique token */
    coap_session_new_token(coap_session, &tokenlength, token);
    if (coap_add_token(request, tokenlength, token) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_token()");
        return ESP_FAIL;
    }

    char *provision_path = "api/v1/provision";
    if (coap_add_option(request, COAP_OPTION_URI_PATH, strlen(provision_path), (u_char*) provision_path) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_option()");
        return ESP_FAIL;
    }

    if (coap_add_data(request, strlen(content), (unsigned char*) content) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_data_large_request()");
        return ESP_FAIL;
    }

    if (coap_send(coap_session, request) == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "Error en coap_send()");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t coap_client_telemetry_post(char *content) {

    if (optlist_telemetry == NULL) {
        ESP_LOGE(TAG, "optilist_telemetry is null");
        return ESP_FAIL;
    }

    return coap_client_post(content, &optlist_telemetry);
}

esp_err_t coap_set_device_token(char *device_token) {

    esp_err_t err;

    if (device_token == NULL) {
        ESP_LOGE(TAG, "coap_set_device_token: device token must not be null.");
        return ESP_ERR_INVALID_ARG;
    }

    DEVICE_TOKEN = strdup(device_token);
    err = coap_generate_telemetry_optlist();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en coap_generate_telemetry_optlist: %s", esp_err_to_name(err));
        return err;
    }

    err = coap_generate_attributes_optlist();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en coap_generate_attributes_optlist: %s", esp_err_to_name(err));
        return err;
    }

    err = coap_generate_firmware_optlist();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en coap_generate_firmware_optlist: %s", esp_err_to_name(err));
        return err;
    }

    err = coap_client_attributes_observe();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en coap_client_attributes_observe: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t coap_client_fw_download(unsigned char *pdu_token, int chunk, int chunk_size) {

    char buff[128];
    size_t tokenlength;
    coap_pdu_t *request = NULL;

    request = coap_new_pdu(COAP_MESSAGE_CON, COAP_REQUEST_CODE_GET, coap_session);
    if (!request) {
        ESP_LOGE(TAG, "Error en coap_new_pdu()");
        return ESP_FAIL;
    }
    
    /* Add in an unique token */
    coap_session_new_token(coap_session, &tokenlength, pdu_token);
    if (coap_add_token(request, tokenlength, pdu_token) == 0) {
        ESP_LOGE(TAG, "Error en coap_add_token()");
        return ESP_FAIL;
    }

    sprintf(buff, "fw/%s", DEVICE_TOKEN);
    ESP_LOGI(TAG, "COAP fiwmare update URI: %s", buff);
    if (coap_add_option(request, COAP_OPTION_URI_PATH, strlen(buff), (u_char*) buff) == 0) {
        ESP_LOGE(TAG, "Error en coap add option");
        return ESP_FAIL;
    }

    sprintf(buff, "size=%d", chunk_size);
    if (coap_add_option(request, COAP_OPTION_URI_QUERY, strlen(buff), (u_char*) buff) == 0) {
        ESP_LOGE(TAG, "Error en coap add option");
        return ESP_FAIL;
    }

    sprintf(buff, "chunk=%d", chunk);
    if (coap_add_option(request, COAP_OPTION_URI_QUERY, strlen(buff), (u_char*) buff) == 0) {
        ESP_LOGE(TAG, "Error en coap add option");
        return ESP_FAIL;
    }

    if (coap_send(coap_session, request) == COAP_INVALID_MID) {
        ESP_LOGE(TAG, "Error en coap_send()");
        return ESP_FAIL;
    }
    
    return ESP_OK;        
}

#endif