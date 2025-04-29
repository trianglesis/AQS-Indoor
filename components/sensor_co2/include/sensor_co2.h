#pragma once
#include <string.h>
#include "esp_log.h"


#define I2C_SCD40_ADDRESS CONFIG_I2C_SCD40_ADDRESS

void sensor_co2(void);
