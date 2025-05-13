#pragma once
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_system.h"

#include "esp_log.h"

#include "esp_attr.h"
#include "esp_sleep.h"

#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"


#define SNTP_TIME_SERVER CONFIG_SNTP_TIME_SERVER

void time_service_info(void);
void set_timezone(void);

esp_err_t start_ntp_service(void);
