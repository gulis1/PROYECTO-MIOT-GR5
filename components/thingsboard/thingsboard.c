#include <esp_log.h>
#include <esp_event.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <cJSON.h>
#include <esp_ota_ops.h>
#include <math.h>

#ifdef CONFIG_USE_COAP
    #include <lwip/sockets.h>
    #include <coap3/coap.h>
    #include "coap_client.h"
#elif CONFIG_USE_MQTT
    #include "mqtt_api.h"
#endif

#include "thingsboard.h"

const static char *TAG = "thingsboard";

static char *NVS_DEVICE_TOKEN_KEY = "device_token";
static char *DEVICE_TOKEN = NULL;
static nvs_handle_t nvshandle;

ESP_EVENT_DEFINE_BASE(THINGSBOARD_EVENT);

extern const uint8_t cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t cert_pem_end[]   asm("_binary_cert_pem_end");

#ifdef CONFIG_USE_COAP
uint8_t COAP_TOKEN_FW_UPDATE[8] = {0};
uint8_t COAP_TOKEN_PROVISION[8] = {0};
uint8_t COAP_TOKEN_ATTRIBUTES[8] = {0};
uint8_t COAP_TOKEN_RPC[8] = {0};
#else
int MQTT_FW_UPDATE_REQUEST_ID = 1;
#endif

/*
    COSAS RPC
*/

esp_err_t rpc_request_received(char *request_data, size_t request_len) {

    cJSON *json = cJSON_ParseWithLength(request_data, request_len);
    if (json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *method_json = cJSON_GetObjectItem(json, "method");
    if (method_json == NULL) {
        cJSON_Delete(json);
        return ESP_ERR_INVALID_ARG;
    }

    void *event_data = &json;
    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_RPC_REQUEST, event_data, sizeof(event_data), portMAX_DELAY);
    return ESP_OK;        
}

/*
    COSAS OTA
*/
esp_ota_handle_t ota_handle;
static bool currently_updating = false;
const static uint32_t FW_UPDATE_CHUNK_SIZE = 1024;
static uint32_t fw_chunks_downloaded = 0;
static uint32_t fw_total_chunks = 0;

static void fw_update_send_state(char *state) {

    // Se indica a thingsboard que se está descargando la actualización.
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "fw_state", state);
    char *payload = cJSON_PrintUnformatted(json);
    thingsboard_telemetry_send(payload);

    cJSON_Delete(json);
    cJSON_free(payload);
}

static void fw_update_download_chunk(int chunk) {

    esp_err_t err = ESP_FAIL;

    #ifdef CONFIG_USE_COAP
        
        // Probar varias veces a descargar
        for (int i = 0; i < 5; i++) {
            err = coap_client_fw_download((uint8_t*) &COAP_TOKEN_FW_UPDATE, chunk, FW_UPDATE_CHUNK_SIZE);
            if (err == ESP_OK)
                break;
        }

    #else

        char payload_buffer[16];
        char topic_buffer[128];
        int n = snprintf(topic_buffer, sizeof(topic_buffer), "v2/fw/request/%d/chunk/%d", MQTT_FW_UPDATE_REQUEST_ID, chunk);
        if (n >= sizeof(topic_buffer)) {
            ESP_LOGE(TAG, "Failed to download firmware: MQTT topic too long.");
            goto chunk_fail;
        }

        n = snprintf(payload_buffer, sizeof(payload_buffer), "%lu", FW_UPDATE_CHUNK_SIZE);
        if (n >= sizeof(topic_buffer)) {
            ESP_LOGE(TAG, "Chunk size buffer too small.");
            goto chunk_fail;
        }

        for (int i = 0; i < 5; i++) {
            err = mqtt_send(topic_buffer, payload_buffer, 1);
            if (err == ESP_OK)
                break;
        }
chunk_fail:    

    #endif

    if (err != ESP_OK) {
        fw_update_send_state("FAILED");
        currently_updating = false;
        ESP_LOGE(TAG, "Failed to download firmware update chunk %d", chunk);
    }
}

static esp_err_t fw_update_downloaded() {

    esp_err_t err;

    ESP_LOGI(TAG, "Actualizacion descargada.");
    fw_update_send_state("DOWNLOADED");

    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_ota_set_boot_partition");
        return err;
    }
    
    fw_update_send_state("UPDATING");
    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_FW_UPDATE_READY, NULL, 0, portMAX_DELAY);
    
    return ESP_OK;
}

static void fw_update_chunk_received(char *data, int data_len) {

    ESP_ERROR_CHECK(esp_ota_write_with_offset(ota_handle, data, data_len, FW_UPDATE_CHUNK_SIZE * fw_chunks_downloaded));
    fw_chunks_downloaded += 1;
    ESP_LOGI(TAG, "Downloaded update chunk %lu/%lu", fw_chunks_downloaded, fw_total_chunks);

    if (fw_chunks_downloaded < fw_total_chunks) {
        fw_update_download_chunk(fw_chunks_downloaded);
    }
    else 
        ESP_ERROR_CHECK(fw_update_downloaded());
}

static esp_err_t fw_update_begin(int fw_size) {

    currently_updating = true;
    ESP_LOGI(TAG, "Iniciando actualizacion. Nueva versión: %d bytes", fw_size);
    fw_chunks_downloaded = 0;
    fw_total_chunks = (int) ceil((double) fw_size / (double) FW_UPDATE_CHUNK_SIZE);
    
    // Se indica a thingsboard que se está descargando la actualización.
    fw_update_send_state("DOWNLOADING");
    fw_update_download_chunk(0);

    return ESP_OK;
}

/*
    COSAS PROVISION
*/
static cJSON* generate_provision_json() {

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "provisionDeviceKey", CONFIG_THINGSBOARD_PROVISION_DEVICE_KEY);
    cJSON_AddStringToObject(json, "provisionDeviceSecret", CONFIG_THINGSBOARD_PROVISION_DEVICE_SECRET);
    
    return json;
}

static esp_err_t parse_received_device_token(char* response, int response_len) {

    cJSON *response_json = cJSON_ParseWithLength(response, response_len);
    cJSON *token_object = cJSON_GetObjectItem(response_json, "credentialsValue");
    if (token_object == NULL) {
        ESP_LOGE(TAG, "Invalid provision JSON received");
        return ESP_FAIL;
    }
    
    DEVICE_TOKEN = strdup(cJSON_GetStringValue(token_object));
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

    cJSON_Delete(response_json);
    return ESP_OK;
}


/*
    COSAS ATRIBUTOS
*/
static esp_err_t parse_attributes(cJSON *json) {
    
    // Comprobamos actualizaciones de firmware.
    cJSON *fw_version_json = cJSON_GetObjectItem(json, "fw_version");
    cJSON *fw_size_json = cJSON_GetObjectItem(json, "fw_size");
    cJSON *fw_title_json = cJSON_GetObjectItem(json, "fw_title");
    if (fw_version_json != NULL && fw_size_json != NULL && fw_title_json != NULL) {
        
        char *fw_version = cJSON_GetStringValue(fw_version_json);
        int fw_size =  (int) cJSON_GetNumberValue(fw_size_json);
        if (!currently_updating && strcmp(fw_version, CONFIG_APP_PROJECT_VER) != 0) {
            ESP_ERROR_CHECK(fw_update_begin(fw_size));
        }
        else {
            ESP_LOGI(TAG, "La versión del firmware es la más actual.");

            // Avisar a thingsboard de que ya estamos actualizados.
            cJSON *json_updated = cJSON_CreateObject();
            cJSON_AddStringToObject(json_updated, "current_fw_title", cJSON_GetStringValue(fw_title_json));
            cJSON_AddStringToObject(json_updated, "current_fw_version", CONFIG_APP_PROJECT_VER);
            cJSON_AddStringToObject(json_updated, "fw_state", "UPDATED");
            char *payload = cJSON_PrintUnformatted(json_updated);
            thingsboard_telemetry_send(payload);
            cJSON_Delete(json_updated);
            cJSON_free(payload);
        }
    }
    
    // Al recibir datos de thingsboard marcamos la actualización como válida.
    ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());

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

    esp_mqtt_event_handle_t mqtt_event = event_data;
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
                cJSON_Delete(json);
                cJSON_free(json_payload);
                ESP_LOGI(TAG, "Enviada solicitud de provisionamiento");
            }
            else {
                // Ya estamos provisionados.
                mqtt_subscribe("v1/devices/me/attributes");
                mqtt_subscribe("v2/fw/response/+/chunk/+");
                esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
            }
            break;
        
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "Could not connect to MQTT broker.");
            esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_DATA:

            if (mqtt_event->topic_len == 0) {
                ESP_LOGE(TAG, "WHAAAAAAAAAAAAAAAAAAAAAAAT. Dta len: %d", mqtt_event->data_len);
                return;
            }
            ESP_LOGI(TAG, "Recbido mensaje a topic MQTT:\n%.*s\n", mqtt_event->topic_len, mqtt_event->topic);
            
            if (strncmp(mqtt_event->topic, "/provision/response", mqtt_event->topic_len) == 0) { 
                ESP_LOGI(TAG, "Mensaje MQTT:\n%.*s\n", mqtt_event->data_len, mqtt_event->data);
                ESP_ERROR_CHECK(parse_received_device_token(mqtt_event->data, mqtt_event->data_len));
                ESP_LOGI(TAG, "Received device token: %s", DEVICE_TOKEN);
                xTaskCreate(restart_mqtt_client, "Restart mqtt client", 2048, NULL, 8, NULL);
            }

            else if (strncmp(mqtt_event->topic, "v1/devices/me/attributes", mqtt_event->topic_len) == 0
                    || strncmp(mqtt_event->topic, "v1/devices/me/attributes/response", 32) == 0)

            {
                ESP_LOGI(TAG, "Mensaje atributos MQTT:\n%.*s\n", mqtt_event->data_len, mqtt_event->data);
                // Hay que sacar el campo shared de aqui.
                cJSON *json = cJSON_ParseWithLength(mqtt_event->data, mqtt_event->data_len);
                if (json == NULL) {
                    ESP_LOGE(TAG, "Error al parsear JSON recibido");
                    return;
                }
                cJSON *shared = cJSON_GetObjectItem(json, "shared");
                if (shared != NULL)
                    parse_attributes(shared);
                else
                    parse_attributes(json);

                cJSON_Delete(json);
            
            }
            else {
                ESP_LOGI(TAG, "Tamaño mensaje MQTT %d", mqtt_event->data_len);
                fw_update_chunk_received(mqtt_event->data, mqtt_event->data_len);
            }

            break;
        case MQTT_EVENT_SUBSCRIBED:
            mqtt_send("v1/devices/me/attributes/request/1", "{}", 0);

            ESP_LOGI(TAG, "Suscrito a topic MQTT:\n%.*s\n", mqtt_event->topic_len, mqtt_event->topic);
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

            coap_bin_const_t token = coap_pdu_get_token(received);
            if (memcmp(token.s, COAP_TOKEN_PROVISION, token.length) == 0) {

                if (parse_received_device_token((char*) data, data_len) != ESP_OK)
                    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
                else {
                    ESP_LOGI(TAG, "Received device token: %s", DEVICE_TOKEN);
                    if (coap_set_device_token(DEVICE_TOKEN, COAP_TOKEN_ATTRIBUTES, COAP_TOKEN_RPC) != ESP_OK)
                        ESP_LOGE(TAG, "Error en coap_set_device_token");
                    else 
                        esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
                }    
            }

            else if (memcmp(token.s, COAP_TOKEN_FW_UPDATE, token.length) == 0) {
                fw_update_chunk_received((char*) data, data_len);
            }

            else if (memcmp(token.s, COAP_TOKEN_ATTRIBUTES, token.length) == 0) {

                ESP_LOGI(TAG, "Received COAP message (%d bytes):\n%.*s\n", data_len, (int)data_len, data);

                cJSON *json = cJSON_ParseWithLength((const char*)data, data_len);
                if (json == NULL) {
                    ESP_LOGE(TAG, "Error al parsear JSON recibido.");
                    return COAP_RESPONSE_OK;
                }

                parse_attributes(json);
                cJSON_Delete(json);
            }

            else if (memcmp(token.s, COAP_TOKEN_RPC, token.length) == 0) {
                rpc_request_received((char*)data, data_len);
                ESP_LOGI(TAG, "Received COAP RPC message (%d bytes):\n%.*s\n", data_len, (int)data_len, data);
            }

            else {
                ESP_LOGI(TAG, "Unknown COAP message received.");
            }

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

        const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
 
    if (partition == NULL) {
        ESP_LOGE(TAG, "Error en esp_ota_get_next_update_partition");
        return ESP_FAIL;
    }
    
    err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_ota_begin");
        return err;
    }

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
    
            err = coap_set_device_token(DEVICE_TOKEN, COAP_TOKEN_ATTRIBUTES, COAP_TOKEN_RPC);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error en coap_set_device_token: %s", esp_err_to_name(err));
                return ESP_FAIL;
            }
        
            return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
        }

        else {
            cJSON *provision_json = generate_provision_json();
            char *provision_payload = cJSON_PrintUnformatted(provision_json);
            err = coap_client_provision_post(provision_payload, (uint8_t*) &COAP_TOKEN_PROVISION);
    
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
        return coap_client_telemetry_post(msg);
    #elif CONFIG_USE_MQTT
        return mqtt_send("v1/devices/me/telemetry", msg, 0);

    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_stop() {

    currently_updating = false;

    #ifdef CONFIG_USE_COAP
        return coap_client_stop();
    #elif CONFIG_USE_MQTT
        return mqtt_stop();
    #else
        return ESP_FAIL;
    #endif
}

esp_err_t thingsboard_attributes_send(char *content) {
   
    #ifdef CONFIG_USE_COAP
        return coap_client_attributes_post(content);
    #elif CONFIG_USE_MQTT
        return mqtt_send("v1/devices/me/attributes", content, 1);
    #else
        return ESP_FAIL;
    #endif
}