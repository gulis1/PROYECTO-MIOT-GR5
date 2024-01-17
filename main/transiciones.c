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

#include "main.h"
#include "wifi.h"
#include "provision.h"
#include "sensores.h"
#include "sntp_client.h"
#include "power_mngr.h"
#include "thingsboard.h"
#include "bluetooth.h"

const static char *TAG = "transiciones.c";


estado_t trans_estado_inicial(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_PROVISION:
            esp_err_t err;
            prov_info_t *prov = (prov_info_t*) trans.dato;
            
            ESP_ERROR_CHECK(wifi_connect(prov->wifi_ssid, prov->wifi_pass));
            
            err= bluetooth_init_finish_provision(bluetooth_handler);
            if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error en inicializar Bluetooth: %s", esp_err_to_name(err));
            }

            return ESTADO_PROVISIONADO;
            
        default:
            return ESTADO_SIN_PROVISION;
    }

}

estado_t trans_estado_provisionado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_WIFI_READY:
            /*INICAR MEDICION DE TEMPERATURA, HUMEDAD Y AIRE*/
            ESP_ERROR_CHECK(time_sync_start());

            return ESTADO_CONECTADO;
            
        default:
            return ESTADO_PROVISIONADO;
    }
}

estado_t trans_estado_conectado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_SINCRONIZAR:
            ESP_LOGI(TAG,"sincronizacion realizada");
            thingsboard_start();
            return ESTADO_HORA_CONFIGURADA;
            
        default:
        return ESTADO_CONECTADO;

    }
}

estado_t trans_estado_hora_configurada(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_THINGSBOARD_READY:
            ESP_ERROR_CHECK(start_calibracion());
            return ESTADO_THINGSBOARD_READY;
        
        case TRANS_THINGSBOARD_UNAVAILABLE:
            ESP_LOGI(TAG, "ALV");
            esp_restart();
            break;
        default:
            return ESTADO_CONECTADO;
    }
}


estado_t trans_estado_thingsboard_ready(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_CALIBRACION_REALIZADA:
            ESP_ERROR_CHECK(estimacion_de_aforo());
            ESP_ERROR_CHECK(sensores_start());
            ESP_ERROR_CHECK(power_manager_start());
            ESP_LOGI(TAG, "Calibración completada");
            return ESTADO_CALIBRADO;
            
        default:
            return ESTADO_THINGSBOARD_READY;
    }
}

estado_t trans_estado_calibrado(transicion_t trans) {

    char json_buffer[128];
    switch (trans.tipo) {

        case TRANS_LECTURA_SENSORES:

            data_sensores_t *lecturas = trans.dato;
            sprintf(json_buffer, "{'temperatura': %.3f, 'eCO2': %d, 'TVOC': %d}", lecturas->temp_dato, lecturas->CO2_dato, lecturas->TVOC_dato);
            thingsboard_telemetry_send(json_buffer);
            return ESTADO_CALIBRADO;


        case TRANS_LECTURA_BLUETOOTH:
            
            int aforo = (*(int*)trans.dato);
            sprintf(json_buffer, "{'estimacion aforo': %d}", aforo);
            thingsboard_telemetry_send(json_buffer);
            ESP_LOGI(TAG,"%s", json_buffer);

            return ESTADO_CALIBRADO;

        case TRANS_THINGSBOARD_FW_UPDATE:
            esp_restart();
            __unreachable();

        case TRANS_DORMIR:
            deep_sleep();
            __unreachable();

        default:
            return ESTADO_CALIBRADO;
    }
}


