/*  Fichero de transiciones.

    En este fichero van las funciones que aplicar cuando
    se recibe una transición para cada uno de los posibles
    estados en los que se puede encontrar la máquina.

    Todas las funciones reciben un argumento de tipo
    "transicion_t" (la última transición leída de la cola) 
    y tienen que devolver el nuevo estado en el que se 
    encontrará la máquina cuando se aplique la transición.
*/

#include <esp_log.h>
#include <esp_err.h>
#include <cJSON.h>

#include "main.h"
#include "wifi.h"
#include "provision.h"
#include "sensores.h"
#include "sntp_client.h"
#include "power_mngr.h"
#include "thingsboard.h"
#include "bluetooth.h"
#include "boton.h"

const static char *TAG = "transiciones.c";

void process_rpc_request(cJSON *request);

estado_t trans_estado_inicial(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_PROVISION:
            ESP_ERROR_CHECK(bluetooth_init_finish_provision(bluetooth_handler));
            stop_provisioning();
            wifi_disconnect();
            ESP_ERROR_CHECK(start_calibracion());
            return ESTADO_PROVISIONADO;
            
        default:
            return ESTADO_SIN_PROVISION;
    }

}

estado_t trans_estado_provisionado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_CALIBRACION_REALIZADA:

            ESP_ERROR_CHECK(estimacion_de_aforo());
            ESP_ERROR_CHECK(sensores_start());
            
            prov_info_t *wifi_info = get_wifi_info();
            ESP_ERROR_CHECK(wifi_connect(wifi_info->wifi_ssid, wifi_info->wifi_pass));
            return ESTADO_CALIBRADO;
            
        default:
            return ESTADO_PROVISIONADO;
    }
}

estado_t trans_estado_calibrado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_WIFI_READY:
            
            ESP_ERROR_CHECK(time_sync_start());
            return ESTADO_CONECTADO;
        
        default:
            return ESTADO_CALIBRADO;

    }
}

estado_t trans_estado_conectado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_SINCRONIZAR:
            ESP_LOGI(TAG,"sincronizacion realizada");
            ESP_ERROR_CHECK(power_manager_start());
            ESP_ERROR_CHECK(thingsboard_start());
            return ESTADO_HORA_CONFIGURADA;
            
        default:
            return ESTADO_CONECTADO;

    }
}

estado_t trans_estado_hora_configurada(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_THINGSBOARD_READY:

            prov_info_t *prov_info = get_wifi_info();
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "piso", (double) atoff(prov_info->data_piso));
            cJSON_AddStringToObject(json, "aula", prov_info->data_aula);
            char *payload = cJSON_PrintUnformatted(json);
            ESP_ERROR_CHECK(thingsboard_attributes_send(payload));
            cJSON_free(payload);
            cJSON_Delete(json);
            return ESTADO_THINGSBOARD_READY;
        
        case TRANS_THINGSBOARD_UNAVAILABLE:
            ESP_LOGE(TAG, "No se ha podido conectar a thingsboard.");
            esp_restart();
            break;
        default:
            return ESTADO_HORA_CONFIGURADA;
    }
}

estado_t trans_estado_thingsboard_ready(transicion_t trans) {

    char json_buffer[128];
    switch (trans.tipo) {

        case TRANS_LECTURA_SENSORES:

            data_sensores_t *lecturas = trans.dato;
            sprintf(json_buffer, "{'temperatura': %.3f, humedad: %.3f, 'eCO2': %d, 'TVOC': %d}", lecturas->temp_dato,lecturas->hum_dato, lecturas->CO2_dato, lecturas->TVOC_dato);
            thingsboard_telemetry_send(json_buffer);
            return ESTADO_THINGSBOARD_READY;

        case TRANS_LECTURA_BLUETOOTH:
            
            int aforo = (*(int*)trans.dato);
            sprintf(json_buffer, "{'estimacion aforo': %d}", aforo);
            thingsboard_telemetry_send(json_buffer);
            ESP_LOGI(TAG,"%s", json_buffer);
            return ESTADO_THINGSBOARD_READY;

        case TRANS_THINGSBOARD_RPC_REQUEST:
            cJSON *json = trans.dato;
            process_rpc_request(json);
            return ESTADO_THINGSBOARD_READY;

        case TRANS_WIFI_DISCONECT:
            ESP_LOGW(TAG, "Se ha caido el wifi");
            thingsboard_stop();
            return ESTADO_CALIBRADO;            

        case TRANS_THINGSBOARD_FW_UPDATE:
            esp_restart();
            __unreachable();

        case TRANS_DORMIR:
            deep_sleep();
            __unreachable();

        default:
            return ESTADO_THINGSBOARD_READY;
    }
}

void trans_estado_to_erase(transicion_t trans){

    switch (trans.tipo) {
        case TRANS_ERASE_FLASH:
            ESP_ERROR_CHECK(erase_flash());
            break;
        default:
            break;
        
    }
}


void process_rpc_request(cJSON *request) {


    int request_id = (int) cJSON_GetNumberValue(cJSON_GetObjectItem(request, "id"));
    cJSON *method = cJSON_GetObjectItem(request, "method");
    if (method == NULL) {
        cJSON_Delete(request);
        return;
    }

    char *method_name = cJSON_GetStringValue(method);
    if (strcmp(method_name, "setPeriodoSensor") == 0) {
        int p = (int) cJSON_GetNumberValue(cJSON_GetObjectItem(request, "params"));
        sensores_set_periodo(p);
    }

    else if (strcmp(method_name, "getPeriodoSensor") == 0) {
        char response_buff[64];
        sprintf(response_buff, "%d", sensores_get_periodo());
        ESP_LOGI(TAG, "Frecuencia: %s", response_buff);
        thingsboard_send_rpc_response(request_id, response_buff);
    }

    else if (strcmp(method_name, "getAforoActivo") == 0) {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddBoolToObject(json, "enabled", aforo_activo());
        char *payload = cJSON_PrintUnformatted(json);
        thingsboard_send_rpc_response(request_id, payload);

        cJSON_free(payload);
        cJSON_Delete(json);
    }

    else if (strcmp(method_name, "toggleAforo") == 0) {
        if (aforo_activo()) aforo_stop();
        else estimacion_de_aforo();
    }

    cJSON_Delete(request);
}


