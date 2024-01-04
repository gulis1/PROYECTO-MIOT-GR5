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

const static char *VERSION_TMP = "1.0.0";
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
#endif



/*
    COSAS OTA
*/
esp_ota_handle_t ota_handle;
const static uint32_t FW_UPDATE_CHUNK_SIZE = 1024;
static uint32_t fw_chunks_downloaded = 0;
static uint32_t fw_total_chunks = 0;

static esp_err_t fw_update_begin(int fw_size) {

    ESP_LOGI(TAG, "Iniciando actualizacion. Nueva versión: %d bytes", fw_size);
    fw_chunks_downloaded = 0;
    fw_total_chunks = (int) ceil((double) fw_size / (double) FW_UPDATE_CHUNK_SIZE);

    #ifdef CONFIG_USE_COAP
        return coap_client_fw_download((uint8_t*) &COAP_TOKEN_FW_UPDATE, 0, FW_UPDATE_CHUNK_SIZE);
    #endif

    return ESP_OK;
}

static esp_err_t fw_update_downloaded() {

    esp_err_t err;

    ESP_LOGI(TAG, "Actualizacion descargada.");
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);

    err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_ota_set_boot_partition");
        return err;
    }

    esp_restart();
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


/*
    COSAS ATRIBUTOS
*/
static esp_err_t parse_attributes(const unsigned char *received, int len) {
    
    cJSON *json = cJSON_ParseWithLength((const char*)received, len);
    if (json == NULL) {
        ESP_LOGE(TAG, "Error al parsear el JSON con los atributos.");
        return ESP_ERR_INVALID_ARG;
    }

    // Comprobamos actualizaciones de firmware.
    cJSON *fw_version_json = cJSON_GetObjectItem(json, "fw_version");
    cJSON *fw_version_size = cJSON_GetObjectItem(json, "fw_size");
    if (fw_version_json != NULL && fw_version_size != NULL) {
        
        char *fw_version = cJSON_GetStringValue(fw_version_json);
        int fw_size =  (int) cJSON_GetNumberValue(fw_version_size);
        if (strcmp(fw_version, VERSION_TMP) != 0) {
            ESP_ERROR_CHECK(fw_update_begin(fw_size));
        }
        else
            ESP_LOGI(TAG, "La versión del firmware es la más actual.");
    }
    
    cJSON_Delete(json);
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

            coap_bin_const_t token = coap_pdu_get_token(received);
            if (memcmp(token.s, COAP_TOKEN_PROVISION, token.length) == 0) {

                if (parse_received_device_token((char*) data, data_len) != ESP_OK)
                    esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
                else {
                    ESP_LOGI(TAG, "Received device token: %s", DEVICE_TOKEN);
                    if (coap_set_device_token(DEVICE_TOKEN) != ESP_OK)
                        ESP_LOGE(TAG, "Error en coap_set_device_token");
                    else 
                        esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_READY, NULL, 0, portMAX_DELAY);
                }    
            }

            else if (memcmp(token.s, COAP_TOKEN_FW_UPDATE, token.length) == 0) {

                ESP_ERROR_CHECK(esp_ota_write_with_offset(ota_handle, data, data_len, FW_UPDATE_CHUNK_SIZE * fw_chunks_downloaded));
                fw_chunks_downloaded += 1;
                ESP_LOGI(TAG, "Downloaded update chunk %lu/%lu", fw_chunks_downloaded, fw_total_chunks);
                if (fw_chunks_downloaded < fw_total_chunks) {
                    ESP_ERROR_CHECK(coap_client_fw_download((uint8_t*) &COAP_TOKEN_FW_UPDATE, fw_chunks_downloaded, FW_UPDATE_CHUNK_SIZE));
                }
                else 
                    ESP_ERROR_CHECK(fw_update_downloaded());

            }

            else {
                // Suponemos que se trata de un reporte de cambio
                // en los atributos.
                ESP_LOGI(TAG, "Received COAP message (%d bytes):\n%.*s\n", data_len, (int)data_len, data);
                parse_attributes(data, data_len);
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

    ESP_LOGI(TAG, "Version actual del firmware: %s", VERSION_TMP);

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
        // Iniciación MQTT. Se le pasa el handler de los eventos
        // MQTT para que se registre.
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

    esp_err_t err;
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "AAA partition: 0x%x", (int) partition->address);
    ESP_LOGI(TAG, "AAA partition label: %s", partition->label);
    if (partition == NULL) {
        ESP_LOGE(TAG, "Error en esp_ota_get_next_update_partition");
        return ESP_FAIL;
    }
    
    err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en esp_ota_begin");
        return err;
    }

    #if CONFIG_USE_MQTT
        return mqtt_start();
    #elif CONFIG_USE_COAP
        err = coap_client_start();
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

    #ifdef CONFIG_USE_COAP
        return esp_event_post(THINGSBOARD_EVENT, THINGSBOARD_EVENT_UNAVAILABLE, NULL, 0, portMAX_DELAY);
    #elif CONFIG_USE_MQTT
        return mqtt_stop();
    #else
        return ESP_FAIL;
    #endif
}
