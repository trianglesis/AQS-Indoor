#pragma once
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sqlite3.h"
#include "sqllib.h"

#define DB_ROOT  CONFIG_SD_MOUNT_POINT

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
void battery_stats();
void co2_stats();
void bme680_stats();