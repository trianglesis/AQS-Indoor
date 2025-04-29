#pragma once
#include <stdio.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2c_master.h"

#define I2C_PORT 0

#define COMMON_SDA_PIN CONFIG_COMMON_SDA_PIN
#define COMMON_SCL_PIN CONFIG_COMMON_SCL_PIN
#define I2C_FREQ_HZ CONFIG_I2C_FREQ_HZ

extern i2c_master_bus_handle_t bus_handle;

void i2c_driver(void);
