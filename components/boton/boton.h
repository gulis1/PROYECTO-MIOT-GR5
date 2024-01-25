#ifndef BOTON
#define BOTON


#include <esp_event.h>


enum {
     BOTON_PRESIONADO
};

ESP_EVENT_DECLARE_BASE(BOTON_EVENT);


esp_err_t boton_init(void *boton_handler);
esp_err_t erase_flash();

#endif