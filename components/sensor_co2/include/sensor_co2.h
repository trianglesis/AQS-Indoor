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

#include "sqlite_driver.h"

#define I2C_SCD40_ADDRESS                  CONFIG_I2C_SCD40_ADDRESS
#define SCD40_SDA_PIN                      CONFIG_COMMON_SDA_PIN
#define SCD40_SCL_PIN                      CONFIG_COMMON_SCL_PIN
// TODO: Change to minutes: 1 minute X 60 seconds X 1000 miliseconds
// Sensor can only provide it once for 5 sec!
#define CO2_MEASUREMENT_FREQ                CONFIG_CO2_MEASUREMENT_FREQ  
#define CO2_LED_UPDATE_FREQ                 CONFIG_CO2_LED_UPDATE_FREQ

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
Update LED colour depending on CO2 PPM level
In place, reading from the queue/
*/
void led_co2(void * pvParameters);

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