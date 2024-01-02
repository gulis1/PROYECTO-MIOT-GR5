#ifndef BLUETOOTH
#define BLUETOOTH

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
// #include "nvs.h"
// #include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <math.h>
#include <esp_event.h>
#include <esp_timer.h>

enum {
    ESTIMACION_AFORO
};

ESP_EVENT_DECLARE_BASE(BLUETOOTH_CONFIG_EVENT);

esp_err_t bluetooth_init(void *bluetooth_handler);
esp_err_t estimacion_de_aforo();

#endif