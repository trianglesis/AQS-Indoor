#pragma once
#include <string.h>
#include "esp_log.h"

#include "sqlite3.h"
#include "sqllib.h"

#include "card_driver.h"


#define DB_ROOT                     SD_MOUNT_POINT

extern MessageBufferHandle_t        xMessageBufferQuery;


esp_err_t setup_db(void);

esp_err_t check_or_create_table_battery(void);
esp_err_t battery_stats(
    int adc_raw, 
    int voltage, 
    int voltage_m, 
    int percentage, 
    int max_masured_voltage, 
    int measure_freq, 
    int loop_count);