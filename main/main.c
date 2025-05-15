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
#include "wifi.h" // Battery power via cheap bust converter won't work: brownout

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


void app_main(void) {
    //Allow other core to finish initialization
    const uint32_t free_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "Init...");
    
    /* Startup sequence:
        0. SDCard should start early!
            web pages
            logs
            database
            etc (and future config and calibration data?).
        0. LittleFS 
            initial web pages
            Boot counters and other non secure data
        1. Master bus init.
            1.1 Init SCD4X sensor, queue and task.
            1.1.1 SCD4X also init LED on the board and control its colour.
            1.2 Init BME680 sensor, queue and task.
        2. Init battery mesurement with ADC, qeue and task.
        5. Display SPI or I2C.
        6. Wifi - optional, with webserver and captive portal.
    */
    // Startup
    ESP_ERROR_CHECK(card_init());
    ESP_ERROR_CHECK(fs_setup());
    // Adding SQLite

    ESP_ERROR_CHECK(master_bus_init());         // Init I2C master bus
    ESP_ERROR_CHECK(scd40_sensor_init());       // With queue and task init.
    ESP_ERROR_CHECK(bme680_sensor_init());      // With queue and task init.
    ESP_ERROR_CHECK(battery_one_shot_init());   // With queue and task init.
    
    // Wifi, then Webserver and Captive Portal
    ESP_ERROR_CHECK(wifi_setup());

    // TODO: Try to pass a struct with all vars related to data we want to display
    ESP_ERROR_CHECK(display_init());       // With LVGL and task init. i2c used too
    
    // TODO: Add file logs
    const uint32_t free_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ssize_t delta = free_after - free_before;
    ESP_LOGI(TAG, "INIT MEMORY\n\t\tBefore: %"PRIu32" bytes\n\t\tAfter: %"PRIu32" bytes\n\t\tDelta: %d\n\n", free_before, free_after, delta);
    // End
}
// END