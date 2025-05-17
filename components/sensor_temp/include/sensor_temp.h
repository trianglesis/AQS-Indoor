#pragma once
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#include "esp_log.h"
// My
#include "i2c_driver.h"
// External
#include "bme680.h"

#include "sqlite3.h"
#include "sqllib.h"
#include "sqlite_driver.h"

#define DB_ROOT  CONFIG_SD_MOUNT_POINT


#define BME680_I2C_ADDR_0           CONFIG_BME680_I2C_ADDR_0
#define BME680_I2C_ADDR_1           CONFIG_BME680_I2C_ADDR_1
#define BME680_SDA_PIN              CONFIG_COMMON_SDA_PIN
#define BME680_SCL_PIN              CONFIG_COMMON_SCL_PIN
// TODO: Change to minutes: 1 minute X 60 seconds X 1000 miliseconds
#define BME680_MEASUREMENT_FREQ     CONFIG_BME680_MEASUREMENT_FREQ  // Sensor can only provide it once for 5 sec!

extern QueueHandle_t mq_bme680;

struct BMESensor {
    float temperature;
    float humidity;
    float pressure;
    float resistance;
    uint16_t air_q_index;
    int measure_freq;
};


void sensor_temp(void);

void bme680_reading_fake(void * pvParameters);
void bme680_reading(void * pvParameters);
void create_mq_bme680(void);
void task_bme680(void);

esp_err_t bme680_sensor_init(void);