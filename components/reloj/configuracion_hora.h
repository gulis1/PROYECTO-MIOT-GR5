
#include <esp_event.h>
#include <esp_log.h>

#include <esp_sntp.h>
#include <esp_netif.h>

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

//TODOANGEL: IDENTIFICAR CORRECTAMENTE QUE LIBRERIAS SON MIAS O LAS OTRAS

enum {
    HORA_CONFIGURADA
};

ESP_EVENT_DECLARE_BASE(HORA_CONFIG_EVENT);

esp_err_t sntp_init_hora(void *hora_handler);
esp_err_t init_sincronizacion_hora();
esp_err_t obtain_time(void); //creo que no haria falta