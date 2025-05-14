#pragma once
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"

#include "sqlite3.h"
#include "sqllib.h"

#include "card_driver.h"
#include "battery_driver.h"


#define DB_ROOT                     SD_MOUNT_POINT
extern MessageBufferHandle_t xMessageBufferQuery;


esp_err_t setup_db(void);

/*
int adc_raw, 
int voltage, 
int voltage_m, 
int percentage, 
int max_masured_voltage, 
int measure_freq, 
int loop_count
*/
void battery_stats(struct BattSensor battery_readings);