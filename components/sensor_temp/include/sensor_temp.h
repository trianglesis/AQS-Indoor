#pragma once
#include <string.h>
#include "esp_log.h"

#define BME680_I2C_ADDR_0 CONFIG_BME680_I2C_ADDR_0
#define BME680_I2C_ADDR_1 CONFIG_BME680_I2C_ADDR_1

void sensor_temp(void);
