
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

void sntp_init_hora(void);
void obtain_time()