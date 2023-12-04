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
#include "configuracion_hora.h"
#include "sueno_profundo.h"
#include "thingsboard.h"

const static char *TAG = "transiciones.c";


estado_t trans_estado_inicial(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_PROVISION:

            prov_info_t *prov = (prov_info_t*) trans.dato;
            ESP_ERROR_CHECK(wifi_connect(prov->wifi_ssid, prov->wifi_pass));

            return ESTADO_PROVISIONADO;
            
        default:
            return ESTADO_SIN_PROVISION;
    }

}

estado_t trans_estado_provisionado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_WIFI_READY:
            /*INICAR MEDICION DE TEMPERATURA, HUMEDAD Y AIRE*/
            ESP_ERROR_CHECK(init_sincronizacion_hora());
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
            ESP_ERROR_CHECK(init_calibracion());
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
            ESP_ERROR_CHECK(sensores_start());
            ESP_LOGI(TAG, "Calibración completada");
            return ESTADO_CALIBRADO;
            
        default:
            return ESTADO_THINGSBOARD_READY;
    }
}

estado_t trans_estado_calibrado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_LECTURA_SENSORES:
            char json_buffer[128];
            data_sensores_t *lecturas = trans.dato;
            sprintf(json_buffer, "{'temperatura': %.3f, 'eCO2': %d}", lecturas->temp_dato, lecturas->CO2_dato);
            thingsboard_telemetry_send(json_buffer);
            ESP_LOGI(TAG, "%s", json_buffer);
            return ESTADO_CALIBRADO;

        default:
            return ESTADO_CALIBRADO;
    }
}


