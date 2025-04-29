#include <stdio.h>
// My
#include "i2c_driver.h"
#include "sensor_co2.h"

static const char *TAG = "sensor-co2";


void sensor_co2(void)
{
    printf(" - Init: sensor_co2 empty function call!\n");
    ESP_LOGI(TAG, "SCD40 COMMON_SDA_PIN: %x", COMMON_SDA_PIN);
    ESP_LOGI(TAG, "SCD40 COMMON_SCL_PIN: %x", COMMON_SCL_PIN);
    ESP_LOGI(TAG, "I2C_SCD40_ADDRESS: %x", I2C_SCD40_ADDRESS);

}
