/* CoAP server Example

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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_err.h>
#include <esp_log.h>
#include <coap3/coap.h>

#include "coap_server.h"

const static char *TAG = "CoAP_server";

static coap_context_t *ctx = NULL;
static coap_address_t serv_addr;
// static coap_resource_t *resource = NULL;

// esp_timer_handle_t notify_timer;
// coap_resource_t *time_resource;


// static char espressif_data[100];
// static int espressif_data_len = 0;


// static void notify_observers(void *arg) {
//     coap_resource_notify_observers(time_resource, NULL);
// }

// /*
//  * The resource handler
//  */
// static void
// hnd_espressif_get(coap_resource_t *resource,
//                   coap_session_t *session,
//                   const coap_pdu_t *request,
//                   const coap_string_t *query,
//                   coap_pdu_t *response)
// {
//     coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
//     coap_add_data_large_response(resource, session, request, response,
//                                  query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
//                                  (size_t)espressif_data_len,
//                                  (const u_char *)espressif_data,
//                                  NULL, NULL);
// }

// static void
// hnd_espressif_put(coap_resource_t *resource,
//                   coap_session_t *session,
//                   const coap_pdu_t *request,
//                   const coap_string_t *query,
//                   coap_pdu_t *response)
// {
//     size_t size;
//     size_t offset;
//     size_t total;
//     const unsigned char *data;

//     coap_resource_notify_observers(resource, NULL);

//     if (strcmp (espressif_data, INITIAL_DATA) == 0) {
//         coap_pdu_set_code(response, COAP_RESPONSE_CODE_CREATED);
//     } else {
//         coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
//     }

//     /* coap_get_data_large() sets size to 0 on error */
//     (void)coap_get_data_large(request, &size, &data, &offset, &total);

//     if (size == 0) {      /* re-init */
//         snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
//         espressif_data_len = strlen(espressif_data);
//     } else {
//         espressif_data_len = size > sizeof (espressif_data) ? sizeof (espressif_data) : size;
//         memcpy (espressif_data, data, espressif_data_len);
//     }
// }

// static void
// hnd_espressif_delete(coap_resource_t *resource,
//                      coap_session_t *session,
//                      const coap_pdu_t *request,
//                      const coap_string_t *query,
//                      coap_pdu_t *response)
// {
//     coap_resource_notify_observers(resource, NULL);
//     snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
//     espressif_data_len = strlen(espressif_data);
//     coap_pdu_set_code(response, COAP_RESPONSE_CODE_DELETED);
// }

// #ifdef CONFIG_COAP_MBEDTLS_PKI

// static int
// verify_cn_callback(const char *cn,
//                    const uint8_t *asn1_public_cert,
//                    size_t asn1_length,
//                    coap_session_t *session,
//                    unsigned depth,
//                    int validated,
//                    void *arg
//                   )
// {
//     coap_log(LOG_INFO, "CN '%s' presented by server (%s)\n",
//              cn, depth ? "CA" : "Certificate");
//     return 1;
// }
// #endif /* CONFIG_COAP_MBEDTLS_PKI */

// static void
// coap_log_handler (coap_log_t level, const char *message)
// {
//     uint32_t esp_level = ESP_LOG_INFO;
//     char *cp = strchr(message, '\n');

//     if (cp)
//         ESP_LOG_LEVEL(esp_level, TAG, "%.*s", (int)(cp-message), message);
//     else
//         ESP_LOG_LEVEL(esp_level, TAG, "%s", message);
// }

// static void coap_example_server(void *p)
// {
//     coap_context_t *ctx = NULL;
//     coap_address_t serv_addr;
//     coap_resource_t *resource = NULL;

//     snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
//     espressif_data_len = strlen(espressif_data);
//     coap_set_log_handler(coap_log_handler);
//     coap_set_log_level(EXAMPLE_COAP_LOG_DEFAULT_LEVEL);

//     while (1) {
//         coap_endpoint_t *ep = NULL;
//         unsigned wait_ms;

//         /* Prepare the CoAP server socket */
//         coap_address_init(&serv_addr);
//         serv_addr.addr.sin6.sin6_family = AF_INET6;
//         serv_addr.addr.sin6.sin6_port   = htons(COAP_DEFAULT_PORT);

//         ctx = coap_new_context(NULL);
//         if (!ctx) {
//             ESP_LOGE(TAG, "coap_new_context() failed");
//             continue;
//         }
//         coap_context_set_block_mode(ctx,
//                                     COAP_BLOCK_USE_LIBCOAP|COAP_BLOCK_SINGLE_BODY);


//         ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);
//         if (!ep) {
//             ESP_LOGE(TAG, "udp: coap_new_endpoint() failed");
//             goto clean_up;
//         }
//         if (coap_tcp_is_supported()) {
//             ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_TCP);
//             if (!ep) {
//                 ESP_LOGE(TAG, "tcp: coap_new_endpoint() failed");
//                 goto clean_up;
//             }
//         }
//         resource = coap_resource_init(coap_make_str_const("Espressif"), 0);
//         if (!resource) {
//             ESP_LOGE(TAG, "coap_resource_init() failed");
//             goto clean_up;
//         }
//         coap_register_handler(resource, COAP_REQUEST_GET, hnd_espressif_get);
//         coap_register_handler(resource, COAP_REQUEST_PUT, hnd_espressif_put);
//         coap_register_handler(resource, COAP_REQUEST_DELETE, hnd_espressif_delete);
//         /* We possibly want to Observe the GETs */
//         coap_resource_set_get_observable(resource, 1);
//         coap_add_resource(ctx, resource);

//         // Registramos el resource para la temperatura.
//         coap_resource_t* temp_resource = coap_resource_init(coap_make_str_const("temperature"), 0);
//         if (!temp_resource) {
//             ESP_LOGE(TAG, "coap_resource_init() failed");
//             goto clean_up;
//         }
//         coap_register_handler(temp_resource, COAP_REQUEST_GET, handle_temp_get);
//         coap_add_resource(ctx, temp_resource);


//         // Registramos el resource para el tiempo
//         init_clock();
//         time_resource = coap_resource_init(coap_make_str_const("time"), 0);
//         coap_register_request_handler(time_resource, COAP_REQUEST_GET, hnd_get_fetch_time);
//         coap_register_request_handler(time_resource, COAP_REQUEST_FETCH, hnd_get_fetch_time);
//         coap_register_request_handler(time_resource, COAP_REQUEST_PUT, hnd_put_time);
//         coap_register_request_handler(time_resource, COAP_REQUEST_DELETE, hnd_delete_time);
//         coap_resource_set_get_observable(time_resource, 1);

//         coap_add_attr(time_resource, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
//         coap_add_attr(time_resource, coap_make_str_const("title"), coap_make_str_const("\"Internal Clock\""), 0);
//         coap_add_attr(time_resource, coap_make_str_const("rt"), coap_make_str_const("\"ticks\""), 0);
//         coap_add_attr(time_resource, coap_make_str_const("if"), coap_make_str_const("\"clock\""), 0);
//         coap_add_resource(ctx, time_resource);

//         // Iniciamos timer para notificar.
//         esp_timer_create_args_t timer_args = {
//             .name = "Notify timer",
//             .callback = notify_observers,
//         };
//         ESP_ERROR_CHECK(esp_timer_create(&timer_args, &notify_timer));
//         ESP_ERROR_CHECK(esp_timer_start_periodic(notify_timer, 1 * 1000000));


//         wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
//         while (1) {
//             int result = coap_io_process(ctx, wait_ms);
//             if (result < 0) {
//                 break;
//             } else if (result && (unsigned)result < wait_ms) {
//                 /* decrement if there is a result wait time returned */
//                 wait_ms -= result;
//             }
//             if (result) {
//                 /* result must have been >= wait_ms, so reset wait_ms */
//                 wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
//             }
//         }
//     }
// clean_up:
//     coap_free_context(ctx);
//     coap_cleanup();

//     vTaskDelete(NULL);
// }

// void app_main(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     srand(time(NULL));

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     ESP_ERROR_CHECK(example_connect());

//     xTaskCreate(coap_example_server, "coap", 8 * 1024, NULL, 5, NULL);
// }

esp_err_t coap_server_init() {

   /* Prepare the CoAP server socket */
   coap_address_init(&serv_addr);
   serv_addr.addr.sin6.sin6_family = AF_INET6;
   serv_addr.addr.sin6.sin6_port   = htons(COAP_DEFAULT_PORT);

   ctx = coap_new_context(NULL);
   if (ctx == NULL) {
      ESP_LOGE(TAG, "Error en: coap_new_context()");
      return ESP_FAIL;
   }
   coap_context_set_block_mode(ctx, COAP_BLOCK_USE_LIBCOAP|COAP_BLOCK_SINGLE_BODY);

   coap_endpoint_t *ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);
   if (ep == NULL) {
      ESP_LOGE(TAG, "Error en: coap_new_endpoint()");
      return ESP_FAIL;
   }

   return ESP_OK;
}
