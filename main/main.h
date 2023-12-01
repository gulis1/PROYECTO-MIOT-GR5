#include <esp_event.h>

extern QueueHandle_t fsm_queue;
extern char strftime_buf[64];
extern char hora_actual[80]; //TODOANGEL: se puede poner 64


// extern time_t ahora;
// extern struct tm *miTiempo;
// extern struct tm *mitiempo_en_segundos;
// extern struct tm *mitiempo_en_minutos;
// extern struct tm *mitiempo_en_horas;

// Tipos de datos.
typedef enum {

    ESTADO_SIN_PROVISION,
    ESTADO_PROVISIONADO,
    ESTADO_CONECTADO,
    ESTADO_MQTT_READY,
    ESTADO_HORA_CONFIGURADA,
    //Estado Actual sensorizar
    ESTADO_CALIBRADO,
    //Estado dormido
    ESTADO_DORMIDO

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
    //transcicion para pasas a sincronizar hora
    TRANS_SINCRONIZAR,
    //transcicion para para pasar a sensorizar 
    TRANS_CALIBRACION_REALIZADA,
    TRANS_LECTURA_SENSORES,

    //transcicion para domir
    TRANS_DORMIR

} tipo_transicion_t;

typedef struct {

    tipo_transicion_t tipo;
    void *dato;
} transicion_t;


//auxiliar hora, para obtener la hora actual correcta (despues de sincronizarse) en cada modificacion de estado;
extern void hora(void);


// Handlers para eventos.
void mqtt_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void prov_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void sensores_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void hora_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


// Transiciones
estado_t trans_estado_inicial(transicion_t trans);
estado_t trans_estado_provisionado(transicion_t trans);
estado_t trans_estado_conectado(transicion_t trans);
estado_t trans_estado_mqtt_ready(transicion_t trans);
estado_t trans_estado_calibrado(transicion_t trans);
estado_t trans_estado_hora_configurada(transicion_t trans);
estado_t trans_estado_durmiendo(transicion_t trans);