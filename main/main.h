#include <esp_event.h>

extern QueueHandle_t fsm_queue;

// Tipos de datos.
typedef enum {

    ESTADO_SIN_PROVISION,
    ESTADO_PROVISIONADO,
    ESTADO_CONECTADO,
    ESTADO_HORA_CONFIGURADA,
    ESTADO_THINGSBOARD_READY,
    //Estado Actual sensorizar
    ESTADO_CALIBRADO,
    //Estado dormido

} estado_t;

typedef enum {

    // Transicion provisionamiento ready.
    TRANS_PROVISION,

    // Transiciones MQTT. No se si servirán luego
    // o podremos quitarlos.
    TRANS_WIFI_READY,
    TRANS_WIFI_DISCONECT,

    //Transiciones para pasas a sincronizar hora
    TRANS_SINCRONIZAR,
    TRANS_THINGSBOARD_READY,
    TRANS_THINGSBOARD_UNAVAILABLE,
    TRANS_THINGSBOARD_FW_UPDATE,
    TRANS_THINGSBOARD_RPC_REQUEST,

    //Transiciones para para pasar a sensorizar 
    TRANS_CALIBRACION_REALIZADA,
    TRANS_LECTURA_SENSORES,
    TRANS_LECTURA_BLUETOOTH,

    //Transiciones para domir
    TRANS_DORMIR,

    //Transiciones para estimar aforo
    TRANS_LECTURA_AFORO, //creo que no sirve 

    //Transiciones para el erase flash
    TRANS_ERASE_FLASH


} tipo_transicion_t;

typedef struct {

    tipo_transicion_t tipo;
    void *dato;
} transicion_t;

//auxiliar hora, para obtener la hora actual correcta (despues de sincronizarse) en cada modificacion de estado;
extern void hora(void);
extern esp_err_t comienza_reloj();
extern esp_err_t power_manager_init();


// Handlers para eventos.
void thingsboard_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void prov_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void sensores_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void hora_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void power_manager_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void bluetooth_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void boton_handler (void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


// Transiciones
estado_t trans_estado_inicial(transicion_t trans);
estado_t trans_estado_provisionado(transicion_t trans);
estado_t trans_estado_conectado(transicion_t trans);
estado_t trans_estado_calibrado(transicion_t trans);
estado_t trans_estado_hora_configurada(transicion_t trans);
estado_t trans_estado_thingsboard_ready(transicion_t trans);
estado_t trans_estado_calibrado(transicion_t trans);
void trans_estado_to_erase(transicion_t trans);
