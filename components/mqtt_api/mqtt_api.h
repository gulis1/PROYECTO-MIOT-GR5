#ifndef MQTT_API
#define MQTT_API

#include "esp_event.h"
#include <mqtt_client.h>


/* Inicia el componente MQQT. Recibe como parámetro 
   el event handler para los eventos que postea.*/
esp_err_t mqtt_api_init(void *mqtt_handler);


#endif