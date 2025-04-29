#include <stdio.h>
// My
#include "i2c_driver.h"
#include "sensor_temp.h"

static const char *TAG = "sensor-bme680";

void sensor_temp(void)
{
    printf(" - Init: sensor_temp empty function call!\n\n");
    ESP_LOGI(TAG, "BME680 COMMON_SDA_PIN: %d", COMMON_SDA_PIN);
    ESP_LOGI(TAG, "BME680 COMMON_SCL_PIN: %d", COMMON_SCL_PIN);
    ESP_LOGI(TAG, "BME680_I2C_ADDR_0: %x", BME680_I2C_ADDR_0);
    ESP_LOGI(TAG, "BME680_I2C_ADDR_1: %x", BME680_I2C_ADDR_1);

}
