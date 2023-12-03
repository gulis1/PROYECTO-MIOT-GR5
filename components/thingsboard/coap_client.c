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
#include <coap3/coap.h>



const static char *TAG = "CoAP_client";

static int resp_wait = 1;
static char *COAP_SERVER_URI = NULL;
static coap_context_t *coap_ctx = NULL;
static coap_session_t *coap_session = NULL;


static coap_optlist_t *optlist_telemetry = NULL;

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

esp_err_t coap_client_init(void *message_handler, char *device_token) {

    coap_address_t *dst_addr;
    static coap_uri_t uri;

    
    // TODO: hacer esto bien.
    COAP_SERVER_URI = malloc(256); 
    if (COAP_SERVER_URI == NULL) {
        ESP_LOGE(TAG, "Error en malloc para COAP_SERVER_URI.");
        return ESP_ERR_NO_MEM;
    }
    sprintf(COAP_SERVER_URI, "coap://%s/api/v1/%s/telemetry", CONFIG_THINGSBOARD_URL, device_token);
    ESP_LOGI(TAG, "COAP URL: %s", COAP_SERVER_URI);

    /* Set up the CoAP context */
    coap_ctx = coap_new_context(NULL);
    if (coap_ctx == NULL) {
        ESP_LOGE(TAG, "Erro en coap_new_context()");
        return ESP_FAIL;
    }
    coap_context_set_block_mode(coap_ctx, COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);

    coap_register_response_handler(coap_ctx, message_handler);

    if (coap_split_uri((const uint8_t *)COAP_SERVER_URI, strlen(COAP_SERVER_URI), &uri) == -1) {
        ESP_LOGE(TAG, "Error en coap_split_uri()");
        return ESP_FAIL;
    }

    dst_addr = coap_get_address(&uri);
    if (dst_addr == NULL) {
        ESP_LOGE(TAG, "Error en coap_get_address()");
        return ESP_FAIL;
    }

    coap_session = coap_new_client_session(coap_ctx, NULL, dst_addr,
                                          uri.scheme == COAP_URI_SCHEME_COAP_TCP ? COAP_PROTO_TCP :
                                          COAP_PROTO_UDP);
    if (coap_session == NULL) {
        ESP_LOGE(TAG, "coap_new_client_session() failed");
        return ESP_FAIL;
    }

    u_char buf[4];
    struct coap_optlist_t *opt_format_json = coap_new_optlist(COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof (buf), COAP_MEDIATYPE_APPLICATION_JSON), buf);
    if (opt_format_json == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }
    
    optlist_telemetry = NULL;
    if (coap_insert_optlist(&optlist_telemetry, opt_format_json) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }

    coap_optlist_t *opt_format_path = coap_new_optlist(COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);
    if (opt_format_path == NULL) {
        ESP_LOGE(TAG, "Error en coap_new_optlist()");
        return ESP_FAIL;
    }

    if (coap_insert_optlist(&optlist_telemetry, opt_format_path) == 0) {
        ESP_LOGE(TAG, "Error en coap_insert_optlist()");
        return ESP_FAIL;
    }


    return ESP_OK;
}

esp_err_t coap_client_send(char *content, coap_optlist_t **optlist) {
    
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
    
    if (coap_io_process(coap_ctx, COAP_IO_NO_WAIT) == -1) {
        ESP_LOGE(TAG, "Error en coap_io_process");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t coap_client_telemetry_send(char *content) {
    return coap_client_send(content, &optlist_telemetry);
}