#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_mac.h"

// Function modules
#include "battery_driver.h"
#include "card_driver.h"
#include "display_driver.h"
#include "i2c_driver.h"
#include "littlefs_driver.h"
#include "sensor_co2.h"
#include "sensor_temp.h"
#include "webserver.h"
#include "wifi.h"

// Empty files - placeholders
#include "captive_portal.h"

static const char *TAG = "co2station";

#define SPIN_ITER   350000  //actual CPU cycles consumed will depend on compiler optimization
#define CORE0       0
// only define xCoreID CORE1 as 1 if this is a multiple core processor target, else define it as tskNO_AFFINITY
#define CORE1       ((CONFIG_FREERTOS_NUMBER_OF_CORES > 1) ? 1 : tskNO_AFFINITY)

/*
All data collected from each sensor now can be saved at the SDCard
Create the 'logs' directory at the root.
Decide how to rotate logs.
If there are 100-500 lines - create a new log, and rename an old one?
*/
void save_to_log(void) {

}


void app_main(void)
{
    //Allow other core to finish initialization
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "Init...");

    /*
        1. Start i2c master bus and add devices to warm them up during setup of all other modules
            1.1 Add SCD4x sensor to the BUS, check address, stop measuremenets
                1.1.1 Led init under CO2 Sensor.
            1.2 Add MBE680 sensor to the BUS, stop it and move on
        Proceed with all other modules while sensors are warming up!
    */
    ESP_ERROR_CHECK(master_bus_init());   // Init I2C master bus
    ESP_ERROR_CHECK(battery_one_shot_init()); // With queue and task init.
    ESP_ERROR_CHECK(scd40_sensor_init());   // With queue and task init.
    ESP_ERROR_CHECK(bme680_sensor_init());  // With queue and task init.

    /* Startup sequence:
    2. Wifi setup, AP mode if no known networks found, STA mode if found one.
    3. LittleFS init and read\write sensor states and calibration data
    4. SD Card init and start\continue writing sensors

    x. LCD Display init and show picture
    x. Web Server start as soon as WiFi is ok
    */
    ESP_ERROR_CHECK(wifi_setup());
    ESP_ERROR_CHECK(fs_setup());
    ESP_ERROR_CHECK(card_init());
    ESP_ERROR_CHECK(start_webserver());
    // TODO: Try to pass a struct with all vars related to data we want to display
    ESP_ERROR_CHECK(display_init());       // With LVGL and task init. i2c used too
    
    // Init in order of importance
    captive_portal();   // 2

    // TODO: Add file logs
    // TODO: Check battery power
    // End
}
// END