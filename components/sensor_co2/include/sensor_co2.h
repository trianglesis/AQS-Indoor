#pragma once
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
#include "led_driver.h"
#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"


#define I2C_SCD40_ADDRESS                  CONFIG_I2C_SCD40_ADDRESS
#define SCD40_SDA_PIN                      CONFIG_COMMON_SDA_PIN
#define SCD40_SCL_PIN                      CONFIG_COMMON_SCL_PIN

// Not less that once in 5 seconds!
#define CO2_MEASURE_MIN 5
#if CONFIG_CO2_MEASUREMENT_FREQ_SECONDS < CO2_MEASURE_MIN
// Change to seconds:                       N X 1000ms = 1 second
#define CO2_MEASUREMENT_FREQ                (5 * 1000)
#else
// Change to seconds:                       N X 1000ms = 1 second
#define CO2_MEASUREMENT_FREQ                (CONFIG_CO2_MEASUREMENT_FREQ_SECONDS * 1000)
#endif

#define CO2_LED_UPDATE_FREQ                 (CONFIG_CO2_LED_UPDATE_FREQ_SECONDS * 1000)

extern QueueHandle_t mq_co2;
extern i2c_master_dev_handle_t scd41_handle;

struct SCD4XSensor {
    float temperature;
    float humidity;
    uint16_t co2_ppm;
    int measure_freq;
};

void sensor_co2(void);

/*
Real task measurements from CO2 sensor.
Adding to the queue
*/
void co2_scd4x_reading(void * pvParameters);

/*
Create the queue for CO2 measurements.
Queue len = 1, overwriting, consuming by peeking.
*/
void create_mq_co2(void);

/*
Task to read CO2 measurements coninuously. 
Sleep betweeen measurements.
*/
void task_co2(void);

/*
Add SCD40 device to I2C bus and update device handle glob var.
*/
esp_err_t scd40_sensor_init(void);