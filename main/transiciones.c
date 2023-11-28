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
#include "mqtt_api.h"


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
            mqtt_start();
            return ESTADO_CONECTADO;
            
        default:
            return ESTADO_PROVISIONADO;
    }
}

estado_t trans_estado_conectado(transicion_t trans) {

    switch (trans.tipo) {

        case TRANS_MQTT_CONNECTED:
            return ESTADO_MQTT_READY;
            
        default:
            return ESTADO_CONECTADO;
    }
}