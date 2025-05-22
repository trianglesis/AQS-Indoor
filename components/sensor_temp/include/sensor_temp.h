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


#define BME680_I2C_ADDR_0           CONFIG_BME680_I2C_ADDR_0
#define BME680_I2C_ADDR_1           CONFIG_BME680_I2C_ADDR_1
#define BME680_SDA_PIN              CONFIG_COMMON_SDA_PIN
#define BME680_SCL_PIN              CONFIG_COMMON_SCL_PIN

// Not less that once in 5 seconds!
#define BME680_MEASURE_MIN 5
#if CONFIG_BME680_MEASUREMENT_FREQ_SECONDS < BME680_MEASURE_MIN
// Change to seconds:                       N X 1000ms = 1 second
#define BME680_MEASUREMENT_FREQ             (5 * 1000)
#else
// Change to seconds:                       N X 1000ms = 1 second
#define BME680_MEASUREMENT_FREQ             (CONFIG_BME680_MEASUREMENT_FREQ_SECONDS * 1000)
#endif

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