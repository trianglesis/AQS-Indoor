#include <stdio.h>
#include "i2c_driver.h"

i2c_master_bus_handle_t bus_handle;

static const char *TAG = "i2c-driver-my";


void i2c_driver(void)
{
    printf(" - Init: i2c_driver empty function call!\n");
    ESP_LOGI(TAG, "COMMON_SDA_PIN: %d", COMMON_SDA_PIN);
    ESP_LOGI(TAG, "COMMON_SCL_PIN: %d", COMMON_SCL_PIN);
    ESP_LOGI(TAG, "I2C_FREQ_HZ: %d", I2C_FREQ_HZ);

}
