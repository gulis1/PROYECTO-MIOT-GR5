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

// void Get_current_date_time(char *date_time)
// {
//     char strftime_buf[64];
//     time_t now;
//     struct tm timeinfo;
//     time(&now);
//     localtime_r(&now, &timeinfo);

//     // Set timezone to Indian Standard Time ///cambiar a madrid
//     setenv("TZ", "UTC-05:30", 1);
//     tzset();
//     localtime_r(&now, &timeinfo);

//     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//     ESP_LOGI("HORA", "The current date/time in Delhi is: %s", strftime_buf);
//     strcpy(date_time, strftime_buf);
// }

// // LONG
// void time_sync_notification_cb(struct timeval *tv)
// {
//     ESP_LOGI("SNTP", "Notification of a time synchronization event");
// }

// void initialize_sntp(void)
// {
//     ESP_LOGI("SNTP", "Initializing SNTP");
//     sntp_setoperatingmode(SNTP_OPMODE_POLL);
//     sntp_setservername(0, "pool.ntp.org");
//     sntp_set_time_sync_notification_cb(time_sync_notification_cb);
//     // #ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
//     // sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
//     // #endif
//     sntp_init();
// }

// void obtain_time(void)
// {
//     initialize_sntp();
//     // wait for time to be set
//     time_t now = 0;
//     struct tm timeinfo = {0};
//     int retry = 0;
//     const int retry_count = 10;
//     while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
//     {
//         ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
//     time(&now);
//     localtime_r(&now, &timeinfo);
// }

// void Set_SystemTime_SNTP(){
//     time_t now;
//     struct tm timeinfo;
//     time(&now);
//     localtime_r(&now, &timeinfo);
//     // Is time set? If not, tm_year will be (1970 - 1900).
//     if (timeinfo.tm_year < (2016 - 1900))
//     {
//         ESP_LOGI("SNTP", "Time is not set yet. Connecting to WiFi and getting time over NTP.");
//         obtain_time();
//         // update 'now' variable with current time
//         time(&now);
//     }
// }


void sntp_init_hora(void){
    ESP_LOGI("HORA", "INICIANLIZNADO SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("poolntp.org");
    esp_netif_sntp_init(&config);
}

void obtain_time(){
    sntp_init_hora();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        ESP_LOGI("OBTAIN_TIME", "Esperando.... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}


// //////////////////////////////////////////////////////////////
// /* LwIP SNTP example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */
// #include <string.h>
// #include <time.h>
// #include <sys/time.h>
// #include "esp_system.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_attr.h"
// #include "esp_sleep.h"
// #include "nvs_flash.h"
// #include "protocol_examples_common.h"
// #include "esp_netif_sntp.h"
// #include "lwip/ip_addr.h"
// #include "esp_sntp.h"

// static const char *TAG = "example";

// #ifndef INET6_ADDRSTRLEN
// #define INET6_ADDRSTRLEN 48
// #endif

// /* Variable holding number of times ESP32 restarted since first boot.
//  * It is placed into RTC memory using RTC_DATA_ATTR and
//  * maintains its value when ESP32 wakes from deep sleep.
//  */
// RTC_DATA_ATTR static int boot_count = 0;

// static void obtain_time(void);

// void time_sync_notification_cb(struct timeval *tv)
// {
//     ESP_LOGI(TAG, "Notification of a time synchronization event");
// }

// void app_main(void)
// {
//     ++boot_count;
//     ESP_LOGI(TAG, "Boot count: %d", boot_count);

//     time_t now;
//     struct tm timeinfo;
//     time(&now);
//     localtime_r(&now, &timeinfo);
//     // Is time set? If not, tm_year will be (1970 - 1900).
//     if (timeinfo.tm_year < (2016 - 1900)) {
//         ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
//         obtain_time();
//         // update 'now' variable with current time
//         time(&now);
//     }

//     char strftime_buf[64];

//     // Set timezone to Eastern Standard Time and print local time
//     setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
//     tzset();
//     localtime_r(&now, &timeinfo);
//     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//     ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);

//     // Set timezone to China Standard Time
//     setenv("TZ", "CST-8", 1);
//     tzset();
//     localtime_r(&now, &timeinfo);
//     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//     ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

//     if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
//         struct timeval outdelta;
//         while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
//             adjtime(NULL, &outdelta);
//             ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %jd sec: %li ms: %li us",
//                         (intmax_t)outdelta.tv_sec,
//                         outdelta.tv_usec/1000,
//                         outdelta.tv_usec%1000);
//             vTaskDelay(2000 / portTICK_PERIOD_MS);
//         }
//     }

//     const int deep_sleep_sec = 10;
//     ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
//     esp_deep_sleep(1000000LL * deep_sleep_sec);
// }

// static void obtain_time(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK( esp_event_loop_create_default() );

//     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//      * Read "Establishing Wi-Fi or Ethernet Connection" section in
//      * examples/protocols/README.md for more information about this function.
//      */
//     ESP_ERROR_CHECK(example_connect());

//     ESP_LOGI(TAG, "Initializing and starting SNTP");
//     /*
//      * This is the basic default config with one server and starting the service
//      */
//     esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);

//     config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want

//     esp_netif_sntp_init(&config);

//     // wait for time to be set
//     time_t now = 0;
//     struct tm timeinfo = { 0 };
//     int retry = 0;
//     const int retry_count = 15;
//     while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
//         ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
//     }
//     time(&now);
//     localtime_r(&now, &timeinfo);

//     ESP_ERROR_CHECK( example_disconnect() );
//     esp_netif_sntp_deinit();
// }