#ifndef SENSORES
#define SENSORES

#include <stdio.h>
#include <string.h>

#include <esp_types.h>
#include <sdkconfig.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_err.h>

#include <esp_event.h>

enum{
    SENSORES_ENVIAN_DATO,
};

ESP_EVENT_DECLARE_BASE(SENSORES_EVENT);



esp_err_t sensores_init(void *sensores_handler);
void sensores_start();
void monitor_stop();


#endif