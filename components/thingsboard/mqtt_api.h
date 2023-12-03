#ifndef MQTT_API
#define MQTT_API

#include "esp_event.h"
#include <mqtt_client.h>


/* Inicia el componente MQQT. Recibe como parámetro 
   el event handler para los eventos que postea.*/
esp_err_t mqtt_init(void *mqtt_handler, char *device_token);

esp_err_t mqtt_start();
esp_err_t mqtt_stop();
esp_err_t mqtt_send(char *topic, char *data, int qos);
esp_err_t mqtt_subscribe(char *topic);
#endif