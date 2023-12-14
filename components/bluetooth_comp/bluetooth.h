
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_system.h"
// #include "esp_log.h"
// //#include "nvs_flash.h"
// //#include <esp_event.h>
// #include "esp_bt.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gatts_api.h"
// #include "esp_bt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"
// #include "sdkconfig.h"

#include <string.h>
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "bta/bta_api.h"
#include "common/bt_trace.h"
#include "btc/btc_manage.h"
#include "btc_gap_ble.h"
#include "btc/btc_ble_storage.h"

enum {
    ESTIMACION_AFORO
};

ESP_EVENT_DECLARE_BASE(BLUETOOTH_CONFIG_EVENT);


esp_err_t bluetooth_init(void *bluetooth_handler);
esp_err_t bluetooth_escaneo();
