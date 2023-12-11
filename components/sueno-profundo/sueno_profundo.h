#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"

///timer wakeup

#include "esp_check.h"
#include "esp_sleep.h"


enum {
     PASAR_A_DORMIR
};

ESP_EVENT_DECLARE_BASE(DEEP_SLEEP_EVENT);


esp_err_t init_register_timer_wakeup(void *hora_handler);
esp_err_t deep_sleep();
