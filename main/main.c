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

// Empty files - placeholders
#include "littlefs_driver.h"
#include "captive_portal.h"
#include "card_driver.h"
#include "display_driver.h"
#include "i2c_driver.h"
#include "lvgl_driver.h"
#include "sensor_co2.h"
#include "sensor_temp.h"
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
    
    // Init in order of importance
    wifi();             // 1
    wifi_setup();       // 1

    captive_portal();   // 2
    littlefs_driver();  // 3
    card_driver();      // 4
    webserver();        // 5
    display_driver();   // 6
    lvgl_driver();      // 7
    ui_init_fake();     // 8
    i2c_driver();       // 9
    sensor_co2();       // 10
    sensor_temp();      // 11

    // Tasks add

    // End
}

// END