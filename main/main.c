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

/*
Custom components
In order of importance during init
*/
// Function modules
#include "wifi.h"
#include "i2c_driver.h"
#include "sensor_co2.h"
#include "sensor_temp.h"

// Empty files - placeholders
#include "littlefs_driver.h"
#include "captive_portal.h"
#include "card_driver.h"
#include "display_driver.h"
#include "lvgl_driver.h"
#include "webserver.h"

// LVGL locally installed
#include "lvgl.h"
// SquareLine Studio export
#include "ui.h"

static const char *TAG = "co2station";

#define SPIN_ITER   350000  //actual CPU cycles consumed will depend on compiler optimization
#define CORE0       0
// only define xCoreID CORE1 as 1 if this is a multiple core processor target, else define it as tskNO_AFFINITY
#define CORE1       ((CONFIG_FREERTOS_NUMBER_OF_CORES > 1) ? 1 : tskNO_AFFINITY)

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
    ESP_ERROR_CHECK(master_bus_init());
    ESP_ERROR_CHECK(scd40_sensor_init());
    ESP_ERROR_CHECK(bme680_sensor_init());

    /*
        2. Wifi setup, AP mode if no known networks found, STA mode if found one.

        x. LittleFS init and read\write sensor states and calibration data
        x. SD Card init and start\continue writing sensors
        x. LCD Display init and show picture
        x. Web Server start as soon as WiFi is ok
    */
    ESP_ERROR_CHECK(wifi_setup());
    
    // Init in order of importance
    captive_portal();   // 2
    littlefs_driver();  // 3
    card_driver();      // 4
    webserver();        // 5
    
    display_driver();   // 6
    lvgl_driver();      // 7
    ui_init_fake();     // 8

    /*
        Make a queue for each sensor
        Assign a task:
        - Wait 5 seconds before loop
        - Sleep real time betteen measurements with xTaskDelayUntil
    */
    vTaskDelay(pdMS_TO_TICKS(500));
    task_co2();
    task_bme680();

    // End
}

// END