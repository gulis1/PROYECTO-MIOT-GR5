#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <cJSON.h>

#ifdef CONFIG_USE_COAP
    #include <lwip/sockets.h>
    #include <coap3/coap.h>
    #include "coap_client.h"
#elif CONFIG_USE_MQTT
    #include "mqtt_api.h"
#endif

#include "thingsboard.h"

const char *TAG = "thingsboard";

static char *NVS_DEVICE_TOKEN_KEY = "device_token";
static char *DEVICE_TOKEN = NULL;
static nvs_handle_t nvshandle;

ESP_EVENT_DEFINE_BASE(THINGSBOARD_EVENT);

extern const uint8_t cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t cert_pem_end[]   asm("_binary_cert_pem_end");


cJSON* generate_provision_json() {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "provisionDeviceKey", CONFIG_THINGSBOARD_PROVISION_DEVICE_KEY);
    cJSON_AddStringToObject(json, "provisionDeviceSecret", CONFIG_THINGSBOARD_PROVISION_DEVICE_SECRET);
    
    return json;
}

esp_err_t parse_received_device_token(char* response, int response_len) {

    cJSON *response_json = cJSON_ParseWithLength(response, response_len);
    cJSON *token_object = cJSON_GetObjectItem(response_json, "credentialsValue");
    if (token_object == NULL) {
        ESP_LOGE(TAG, "Invalid provision JSON received");
        return ESP_FAIL;
    }
    
    DEVICE_TOKEN = cJSON_GetStringValue(token_object);
    if (DEVICE_TOKEN == NULL) {
        ESP_LOGE(TAG, "Invalid provision JSON received");
        return ESP_FAIL;
    }
    
    esp_err_t err = nvs_set_str(nvshandle, NVS_DEVICE_TOKEN_KEY, DEVICE_TOKEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en nvs_set_str: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Saved received provision token in NVS");
    nvs_close(nvshandle);

    cJSON_free(response_json);
    return ESP_OK;
}

#ifdef CONFIG_USE_MQTT

static void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void restart_mqtt_client() {
    
    ESP_LOGI(TAG, "Reiniciando cliente MQTT...");
    vTaskDelay(2000 / portMAX_DELAY);
    ESP_ERROR_CHECK(mqtt_deinit());
    ESP_ERROR_CHECK(mqtt_init(mqtt_handler, DEVICE_TOKEN, (char*) cert_pem_start));
    ESP_ERROR_CHECK(mqtt_start());
    vTaskDelete(NULL);
}

static void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

    switch (event_id) {

        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(TAG, "Conectado al broker MQTT de Thingsboard.");
            if (DEVICE_TOKEN == NULL) {
                // Si no estamos provisionados, nos suscribimos al topic.
                ESP_LOGI(TAG, "Device token not found, provisioning via Thingsboard.");
                ESP_ERROR_CHECK(mqtt_subscribe("/provision/response"));

            
                ESP_LOGI(TAG, "Suscrito al topic de provisionamiento");
                cJSON *json = generate_provision_json();
                char *json_payload = cJSON_PrintUnformatted(json);

                vTaskDelay(2000 / portTICK_PERIOD_MS); // TODO: revisar esta chapuza.

                mqtt_send("/provision/request", json_payload, 1);
                cJSON_free(json);
                cJSON_free(json_payload);
                ESP_LOGI(TAG, "Enviada solicitud de provisionamiento");
            }
            else {
                // Si ya estamos, enviamos el evento de ready directamente.
                esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
            }
            break;
        
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "Could not connect to MQTT broker.");
            esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_DATA:

            esp_mqtt_event_handle_t mqtt_event = event_data;
            if (strncmp(mqtt_event->topic, "/provision/response", mqtt_event->topic_len) == 0) {  
                ESP_ERROR_CHECK(parse_received_device_token(mqtt_event->data, mqtt_event->data_len));
                ESP_LOGI(TAG, "Received device token: %s", DEVICE_TOKEN);
                xTaskCreate(restart_mqtt_client, "Restart mqtt client", 2048, NULL, 8, NULL);
            }
            
            break;
    }
}
#endif

#ifdef CONFIG_USE_COAP

static coap_response_t coap_handler(coap_session_t *session,
                                    const coap_pdu_t *sent,
                                    const coap_pdu_t *received,
                                    const coap_mid_t mid)
{
    const unsigned char *data = NULL;
    size_t data_len;
    size_t offset;
    size_t total;

    coap_pdu_code_t rcvd_code = coap_pdu_get_code(received);
    if (COAP_RESPONSE_CLASS(rcvd_code) == 2) {

        if (coap_get_data_large(received, &data_len, &data, &offset, &total)) {
            if (data_len != total) {
                ESP_LOGE(TAG, "Incomplete message received.");
                esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
                return COAP_RESPONSE_OK;
            }

            if (DEVICE_TOKEN == NULL) {
                // Provisionamiento recibido.
                if (parse_received_device_token((char*) data, data_len) != ESP_OK) {
                    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
                    return COAP_RESPONSE_OK;
                }

                ESP_LOGI(TAG, "Received device token: %s", DEVICE_TOKEN);

                if (coap_set_device_token(DEVICE_TOKEN) != ESP_OK)
                    ESP_LOGE(TAG, "Error en coap_set_device_token");
                else 
                    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
            }

            else {
                
            }

            ESP_LOGI(TAG, "Received COAP message:\n%.*s\n", (int)data_len, data);
        }
        return COAP_RESPONSE_OK;
    }

    else {
        ESP_LOGE(TAG, "Error code received: %d.%02d", (rcvd_code >> 5), rcvd_code & 0x1F);
        esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
        return COAP_RESPONSE_OK;
    }

    return COAP_RESPONSE_OK;
}
#endif

esp_err_t thingsboard_init(void *handler) {

    esp_err_t err;
    size_t len;

    err = esp_event_handler_register(THINGSBOARD_EVENT, ESP_EVENT_ANY_ID, handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_event_handler_register: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_open("nvs", NVS_READWRITE, &nvshandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en nvs_open: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvshandle, NVS_DEVICE_TOKEN_KEY, NULL, &len);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Thingsboard token found in NVS");
        DEVICE_TOKEN = malloc(len);
        nvs_get_str(nvshandle, NVS_DEVICE_TOKEN_KEY, DEVICE_TOKEN, &len);
        nvs_close(nvshandle);
    }

    else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error en nvs_get_str(): %s", esp_err_to_name(err));
        return err;
    }

    #if CONFIG_USE_COAP
        err = coap_client_init(coap_handler, (char*) cert_pem_start);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en coap_server_init: %s", esp_err_to_name(err));
            return err;
        }
    #elif CONFIG_USE_MQTT
        //Iniciación MQTT. Se le pasa el handler de los eventos
        //MQTT para que se registre.
        err = mqtt_init(mqtt_handler, DEVICE_TOKEN, (char*) cert_pem_start);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en mqtt_api_init: %s", esp_err_to_name(err));
            return err;
        }
    #else
        ESP_LOGE(TAG, "Protocolo mal configurado")
        return ESP_FAIL;
    #endif

        return ESP_OK;
}

esp_err_t thingsboard_start() {


    #if CONFIG_USE_MQTT
        return mqtt_start();
    #elif CONFIG_USE_COAP
        esp_err_t err = coap_client_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en coap_client_start: %s", esp_err_to_name(err));
            return ESP_FAIL;
        }

        if (DEVICE_TOKEN != NULL) {

            err = coap_set_device_token(DEVICE_TOKEN);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error en coap_client_attributes_observe: %s", esp_err_to_name(err));
                return ESP_FAIL;
            }
            return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
        }

        else {
            cJSON *provision_json = generate_provision_json();
            char *provision_payload = cJSON_PrintUnformatted(provision_json);
            err = coap_client_provision_send(provision_payload);
            free(provision_json);
            free(provision_payload);
            return err;
        }

    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_telemetry_send(char *msg) {

    if (DEVICE_TOKEN == NULL) {
        ESP_LOGE(TAG, "Cannot send telemetry: device token is NULL");
        return ESP_FAIL;
    }

    #if CONFIG_USE_COAP
        return coap_client_telemetry_send(msg);
    #elif CONFIG_USE_MQTT
        return mqtt_send("v1/devices/me/telemetry", msg, 0);

    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_stop() {

    #ifdef CONFIG_USE_COAP
        return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
    #elif CONFIG_USE_MQTT
        return mqtt_stop();
    #else
        return ESP_FAIL;
    #endif
}
