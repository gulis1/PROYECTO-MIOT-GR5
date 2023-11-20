#ifndef MQTT_API
#define MQTT_API

#include "esp_event.h"
#include <mqtt_client.h>

#define GPIO_PIN_BOTON 0

// enum {
//     MQTT_MESSAGE_RECEIVED
// };

// ESP_EVENT_DECLARE_BASE(BOTON_EVENT);

/* Inicia el componente MQQT. Recibe como parámetro 
   el event handler para los eventos que postea.*/
esp_err_t mqtt_api_init(void *event_handler);


#endif