#include <esp_event.h>

extern QueueHandle_t fsm_queue;

// Tipos de datos.
typedef enum {

    ESTADO_SIN_PROVISION,
    ESTADO_PROVISIONADO,
    ESTADO_CONECTADO,
    ESTADO_MQTT_READY,,
    //Estado Actual sensorizar
    ESTADO_SENSORIZANDO

} estado_t;

typedef enum {

    // Transicion provisionamiento ready.
    TRANS_PROVISION,

    // Transiciones MQTT. No se si servirán luego
    // o podremos quitarlos.
    TRANS_WIFI_READY,
    TRANS_WIFI_DISCONECT,
    TRANS_MQTT_CONNECTED,
    TRANS_MQTT_DISCONNECTED,
    //transcicion para para pasar a sensorizar 
    TRANS_SENSORIZACION

} tipo_transicion_t;

typedef struct {

    tipo_transicion_t tipo;
    void *dato;
} transicion_t;


// Handlers para eventos.
void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void prov_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


// Transiciones
estado_t trans_estado_inicial(transicion_t trans);
estado_t trans_estado_provisionado(transicion_t trans);
estado_t trans_estado_conectado(transicion_t trans);