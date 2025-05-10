#include "lvgl_driver.h"
#include "ui_load.h"

// Only import sensors queues and structs definition
#include "battery_driver.h"
#include "sensor_co2.h"
#include "sensor_temp.h"
#include "littlefs_driver.h"  // Used/free space at LittleFS
#include "card_driver.h"      // Used/free space at SDCard
#include "wifi.h"             // WiFi Mode and connected users, DHCP IP

static const char *TAG = "lvgl";

lv_disp_t *display;


void lvgl_driver_info(void) {
    printf("\n\n- Init:\t\tLVGL driver debug info!\n");
    // esp_log_level_set("lcd_panel", ESP_LOG_VERBOSE);
    // esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    #ifdef CONFIG_CONNECTION_SPI
    ESP_LOGI(TAG, "Display connected via CONNECTION_SPI: %d", CONNECTION_SPI);
    #elif CONFIG_CONNECTION_I2C
    ESP_LOGI(TAG, "Display connected via CONNECTION_I2C: %d", CONNECTION_I2C);
    #endif // CONNECTIONS SPI/I2C
    ESP_LOGI(TAG, "BUFFER_SIZE: %d", BUFFER_SIZE);
    ESP_LOGI(TAG, "RENDER_MODE: %d", RENDER_MODE);
    ESP_LOGI(TAG, "Display rotation is set to %d degree! \n\t\t - Offsets X: %d Y: %d", ROTATE_DEGREE, Offset_X, Offset_Y);
}

/*
Simple drawings for I2C display
Keep it as fallback option when SQ Line fails
*/
void lvgl_task_i2c(void * pvParameters)  {
    ESP_LOGI(TAG, "Starting LVGL task");

    lv_lock();
    // Create a simple label
    lv_obj_t *co2_lbl = lv_label_create(lv_screen_active());
    lv_obj_t *temp_lbl = lv_label_create(lv_screen_active());
    lv_obj_t *humid_lbl = lv_label_create(lv_screen_active());
    lv_obj_t *pressure_lbl = lv_label_create(lv_screen_active());
    lv_obj_t *aqi_lbl = lv_label_create(lv_screen_active());
    lv_obj_t *volt = lv_label_create(lv_screen_active());

    lv_label_set_text(co2_lbl, "CO2: 8888 ppm");
    lv_label_set_text(temp_lbl, "t: 99 C");
    lv_label_set_text(humid_lbl, "Hum: 100%%");
    lv_label_set_text(pressure_lbl, "999 hpa");
    lv_label_set_text(aqi_lbl, "AQI 99");
    lv_label_set_text(volt, "333 mV");
    
    lv_obj_set_width(co2_lbl, DISP_HOR_RES);
    lv_obj_set_width(temp_lbl, DISP_HOR_RES);
    lv_obj_set_width(humid_lbl, DISP_HOR_RES);
    lv_obj_set_width(pressure_lbl, DISP_HOR_RES);
    lv_obj_set_width(aqi_lbl, DISP_HOR_RES);
    lv_obj_set_width(volt, DISP_HOR_RES);

    lv_obj_set_style_text_align(co2_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(temp_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(humid_lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_align(pressure_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_align(aqi_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_align(volt, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_align(co2_lbl, LV_ALIGN_TOP_LEFT, 0, 0); 
    lv_obj_align(temp_lbl, LV_ALIGN_LEFT_MID, 0, 0); 
    lv_obj_align(humid_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0); 
    lv_obj_align(pressure_lbl, LV_ALIGN_TOP_RIGHT, 0, 0); 
    lv_obj_align(aqi_lbl, LV_ALIGN_BOTTOM_RIGHT, 0, 0); 
    lv_obj_align(volt, LV_ALIGN_BOTTOM_MID, 0, 0); 
    
    lv_unlock();

    vTaskDelay(pdMS_TO_TICKS(500));
    
    int to_wait_ms = 10;
    long curtime = esp_timer_get_time()/1000;
    struct BMESensor bme680_readings; // data type should be same as queue item type
    struct SCD4XSensor scd4x_readings; // data type should be same as queue item type
    struct BattSensor battery_readings; // data type should be same as queue item type
    const TickType_t xTicksToWait = pdMS_TO_TICKS(to_wait_ms);

    // Handle LVGL tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));  // idle between cycles
        lv_task_handler();
        if (esp_timer_get_time()/1000 - curtime > 1000) {
            curtime = esp_timer_get_time()/1000;
        } // Timer

        xQueuePeek(mq_co2, (void *)&scd4x_readings, xTicksToWait);
        xQueuePeek(mq_bme680, (void *)&bme680_readings, xTicksToWait);
        xQueuePeek(mq_batt, (void *)&battery_readings, xTicksToWait);

        lv_lock();
        lv_label_set_text_fmt(co2_lbl, "%d ppm", scd4x_readings.co2_ppm);
        lv_label_set_text_fmt(temp_lbl, "%.0f C", bme680_readings.temperature);
        lv_label_set_text_fmt(humid_lbl, "%.0f %%", bme680_readings.humidity);
        lv_label_set_text_fmt(pressure_lbl, "%.0f hpa", bme680_readings.pressure);
        lv_label_set_text_fmt(aqi_lbl, "AQI %.0d", bme680_readings.air_q_index);
        lv_label_set_text_fmt(volt, "%.0d mV", battery_readings.voltage);
        lv_unlock();
               
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_FREQ));
    } // WHILE
}

/*
Squareline studio drawings
*/
void lvgl_task_i2c_sq_line(void * pvParameters)  {
    ESP_LOGI(TAG, "Starting LVGL i2c display task");

    // Depends on which connection display uses
    load_graphics();  // Same as ui_init();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    int to_wait_ms = 10;
    long curtime = esp_timer_get_time()/1000;
    struct BMESensor bme680_readings; // data type should be same as queue item type
    struct SCD4XSensor scd4x_readings; // data type should be same as queue item type
    struct BattSensor battery_readings; // data type should be same as queue item type
    const TickType_t xTicksToWait = pdMS_TO_TICKS(to_wait_ms);

    // Handle LVGL tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));  // idle between cycles
        lv_task_handler();
        if (esp_timer_get_time()/1000 - curtime > 1000) {
            curtime = esp_timer_get_time()/1000;
        } // Timer

        xQueuePeek(mq_co2, (void *)&scd4x_readings, xTicksToWait);
        xQueuePeek(mq_bme680, (void *)&bme680_readings, xTicksToWait);
        xQueuePeek(mq_batt, (void *)&battery_readings, xTicksToWait);

        // If connection is wrong: just show logs
        #ifdef CONFIG_CONNECTION_I2C
        lv_lock();
        lv_label_set_text_fmt(ui_co2count, "%d", scd4x_readings.co2_ppm);
        lv_label_set_text_fmt(ui_Temperature, "%.0fC", bme680_readings.temperature);
        lv_label_set_text_fmt(ui_Humidity, "%.0f%%", bme680_readings.humidity);
        lv_label_set_text_fmt(ui_Pressure, "%.0fhpa", bme680_readings.pressure);
        lv_label_set_text_fmt(ui_airQuality, "AQI %.0d", bme680_readings.air_q_index);
        lv_label_set_text_fmt(ui_batteryVoltage, "%.0dmV", battery_readings.voltage_m);
        lv_label_set_text_fmt(ui_Battery, "%d%%", battery_readings.percentage);
        // Other
        lv_label_set_text_fmt(ui_LittleFSUsed, "%.0fKB", littlefs_used);
        lv_label_set_text_fmt(ui_SDCardFree, "%.0fGB", sd_free);
        // Network info
        if (wifi_ap_mode == true && found_wifi == false) {
            lv_label_set_text_fmt(ui_Network, "%d", connected_users);
            lv_label_set_text_fmt(ui_NetworkAddress, "%s", ip_string);
            lv_obj_remove_flag(ui_wifiApMode, LV_OBJ_FLAG_HIDDEN);
        } else if (found_wifi == true) {
            lv_obj_add_flag(ui_Network, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(ui_NetworkAddress, "%s", ip_string);
            lv_obj_remove_flag(ui_wifiStaMode, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_Network, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_NetworkAddress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_wifiApMode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_wifiStaMode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_wifiDisabled, LV_OBJ_FLAG_HIDDEN);
        }
        // Battery icon
        if (battery_readings.percentage >= 85) {
            lv_obj_remove_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        } else if (battery_readings.percentage <= 84 && battery_readings.percentage >= 50 ) {
            lv_obj_add_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        } else if (battery_readings.percentage <= 50 && battery_readings.percentage >= 30 ) {
            lv_obj_add_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        } else if (battery_readings.percentage <= 29 && battery_readings.percentage >= 10 ) {
            lv_obj_add_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        } else if (battery_readings.percentage <= 9) {
            lv_obj_add_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_BatteryFull, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_Battery80, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryHalf, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_BatteryLow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_BatteryEmpty, LV_OBJ_FLAG_HIDDEN);
        }

        lv_unlock();
        #else
        ESP_LOGW(TAG, "Cannot start LVGL task for I2C display, it wasn't not set up!");
        #endif // CONFIG_CONNECTION

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_FREQ));
    } // WHILE
}


void lvgl_task_spi_sq_line(void * pvParameters)  {
    ESP_LOGI(TAG, "Starting LVGL spi display task");

    // Depends on which connection display uses
    load_graphics();  // Same as ui_init();
    vTaskDelay(pdMS_TO_TICKS(500));
    int to_wait_ms = 10;
    long curtime = esp_timer_get_time()/1000;
    struct BMESensor bme680_readings; // data type should be same as queue item type
    struct SCD4XSensor scd4x_readings; // data type should be same as queue item type
    struct BattSensor battery_readings; // data type should be same as queue item type
    const TickType_t xTicksToWait = pdMS_TO_TICKS(to_wait_ms);
    // Handle LVGL tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));  // idle between cycles
        lv_task_handler();
        if (esp_timer_get_time()/1000 - curtime > 1000) {
            curtime = esp_timer_get_time()/1000;
        } // Timer

        xQueuePeek(mq_co2, (void *)&scd4x_readings, xTicksToWait);
        xQueuePeek(mq_bme680, (void *)&bme680_readings, xTicksToWait);
        xQueuePeek(mq_batt, (void *)&battery_readings, xTicksToWait);
        
        #ifdef CONFIG_CONNECTION_SPI
        lv_lock();
        // CO2 Arc SDC41
        lv_arc_set_value(ui_ArcCO2, scd4x_readings.co2_ppm);
        lv_label_set_text_fmt(ui_LabelCo2Count, "%d", scd4x_readings.co2_ppm);
        lv_label_set_text(ui_LabelCo2, "CO2");
        lv_label_set_text(ui_LabelCo2Ppm, "ppm");

        // Temerature, humidity etc: BME680
        lv_bar_set_value(ui_BarTemperature, bme680_readings.temperature, LV_ANIM_ON);
        lv_bar_set_value(ui_BarHumidity, bme680_readings.humidity, LV_ANIM_ON);

        lv_label_set_text_fmt(ui_LabelTemperature, "%.0f", bme680_readings.temperature);
        lv_label_set_text_fmt(ui_LabelHumidity, "%.0f", bme680_readings.humidity);
        lv_label_set_text_fmt(ui_LabelPressure, "%.0f", bme680_readings.pressure);
        lv_label_set_text_fmt(ui_LabelAirQualityIndx, "AQI %.0d", bme680_readings.air_q_index);

        // Storage info
        // lv_label_set_text_fmt(ui_Label4, "SD: %ld GB", SDCard_Size);
        lv_label_set_text_fmt(ui_LabelSdFree, "SD: %.0fGB/%.0fGB free", sd_free, sd_total);
        // Hardcode total as string 2Mb and save LCD space
        // lv_label_set_text_fmt(ui_Label5, "LFS: %.0fKB/%.0fKB used", littlefs_used, littlefs_total);
        lv_label_set_text_fmt(ui_LabelLfsUsed, "LFS: %.0fKB/2MB used", littlefs_used);
        
        // Network info
        if (wifi_ap_mode == true && found_wifi == false) {
            // Show Wifi AP icon if it's active and users connected count
            lv_label_set_text_fmt(ui_LabelApUsers, "%d", connected_users);
            lv_obj_remove_flag(ui_ImageAPMode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_LabelApUsers, LV_OBJ_FLAG_HIDDEN);
        } else if (found_wifi == true) {
            // Show WiFi local Icon and IP
            lv_label_set_text_fmt(ui_LabelipAdress, "%s", ip_string);
            lv_obj_remove_flag(ui_ImageLocalWiFI, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_LabelipAdress, LV_OBJ_FLAG_HIDDEN);
            // Hide AP mode icons
            lv_obj_add_flag(ui_LabelApUsers, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_ImageAPMode, LV_OBJ_FLAG_HIDDEN);
        } else {
            // Lables and icons are hidden by default!
            // Hide all other connection icons
            lv_obj_add_flag(ui_LabelApUsers, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_ImageAPMode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_LabelipAdress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_ImageLocalWiFI, LV_OBJ_FLAG_HIDDEN);
            // Show no WiFi Icon
            lv_obj_remove_flag(ui_ImageNoWiFi, LV_OBJ_FLAG_HIDDEN);
        }
        lv_unlock();
        #else
        ESP_LOGW(TAG, "Cannot start LVGL task for SPI display, it wasn't set up!");
        #endif // CONFIG_CONNECTION
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_FREQ));
    } // WHILE
}



/*
LVGL helper functions.
*/
void lvgl_tick_increment(void *arg) {
    // Tell LVGL how many milliseconds have elapsed
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void lvgl_tick_init(void) {
    // Tick interface for LVGL (using esp_timer to generate 5ms periodic event)
    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    esp_timer_handle_t tick_timer = NULL;  // Move to global?
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_increment,
        .name = "LVGL_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    // Next create a task
}

bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    // this gets called when the DMA transfer of the buffer data has completed
    lv_display_t *disp_drv = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp_drv);
    return false;
}

/* 
    Rotate display, when rotated screen in LVGL. Called when driver parameters are updated. 
    There is no HAL?
    Must change Offset Y to X at flush_cb
    Offset_Y 34  // 34 IF ROTATED 270deg
*/
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    
    /* If Rotated And if this is SPI OLED from Waheshare
        https://forum.lvgl.io/t/gestures-are-slow-perceiving-only-detecting-one-of-5-10-tries/18515/86 
        If not - offset will be 0 which is ok
    */
    int x1 = area->x1 + Offset_X;
    int x2 = area->x2 + Offset_X;
    int y1 = area->y1 + Offset_Y;
    int y2 = area->y2 + Offset_Y;
    
    /* This is different draw implementation for MONOCHROMATIC displays */
    #ifdef CONFIG_CONNECTION_I2C
    // This is necessary because LVGL reserves 2 x 4 bytes in the buffer, as these are assumed to be used as a palette. Skip the palette here
    // More information about the monochrome, please refer to https://docs.lvgl.io/9.2/porting/display.html#monochrome-displays
    px_map += LVGL_PALETTE_SIZE;
    // To use LV_COLOR_FORMAT_I1, we need an extra buffer to hold the converted data
    static uint8_t oled_buffer[BUFFER_SIZE];

    uint16_t hor_res = lv_display_get_physical_horizontal_resolution(disp);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            /* The order of bits is MSB first
                        MSB           LSB
               bits      7 6 5 4 3 2 1 0
               pixels    0 1 2 3 4 5 6 7
                        Left         Right
            */
            bool chroma_color = (px_map[(hor_res >> 3) * y  + (x >> 3)] & 1 << (7 - x % 8));

            /* Write to the buffer as required for the display.
            * It writes only 1-bit for monochrome displays mapped vertically.*/
            uint8_t *buf = oled_buffer + hor_res * (y >> 3) + (x);
            if (chroma_color) {
                (*buf) &= ~(1 << (y % 8));
            } else {
                (*buf) |= (1 << (y % 8));
            }
        }
    }
    #endif // CONFIG_CONNECTION
    
    // Panel handle is global var from display_driver
    #ifdef CONFIG_CONNECTION_SPI
    // SPI coloured
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    #elif CONFIG_CONNECTION_I2C
    // I2C mono
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, oled_buffer);
    #endif // CONFIG_CONNECTION
}

/* Sometimes better to hard-set resolution */
void set_resolution(void) {
    lv_display_set_resolution(display, DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_physical_resolution(display, DISP_HOR_RES, DISP_VER_RES);
}

/* Landscape orientation:
    270deg = USB on the left side - landscape orientation
    270deg = USB on the right side - landscape orientation 
90 
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_swap_xy(panel_handle, true);
180
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
    esp_lcd_panel_mirror(panel_handle, true, true);
    esp_lcd_panel_swap_xy(panel_handle, false);
270
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
    esp_lcd_panel_mirror(panel_handle, false, true);
    esp_lcd_panel_swap_xy(panel_handle, true);
See: https://forum.lvgl.io/t/gestures-are-slow-perceiving-only-detecting-one-of-5-10-tries/18515/60
*/
void set_orientation(void) {
    // Rotation
    if (ROTATE_DEGREE == 0) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    } else if (ROTATE_DEGREE == 90) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        esp_lcd_panel_mirror(panel_handle, true, false);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else if (ROTATE_DEGREE == 180) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
        esp_lcd_panel_mirror(panel_handle, true, true);
        esp_lcd_panel_swap_xy(panel_handle, false);
    } else if (ROTATE_DEGREE == 270) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
        esp_lcd_panel_mirror(panel_handle, false, true);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else {
        ESP_LOGI(TAG, "No totation specified, do not use rotation.");
        // lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    }
}

esp_err_t lvgl_init(void) {
    lvgl_driver_info(); // Debug
    lv_init(); // Init

    display = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    if (panel_handle == NULL) {
        ESP_LOGE(TAG, "Panel handle is null!");
    }
    lv_display_set_user_data(display, panel_handle);    // a custom pointer stored with lv_display_t object

    // Use two buffers for both displays setup.
    void* buf1 = NULL;
    void* buf2 = NULL;
    // Different buffer usage
    #ifdef CONFIG_CONNECTION_SPI
    /* 
        SPI config 
        Two buffers for coured OLED, render PARTIAL
    */
    buf1 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);
    buf2 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);
    assert(buf1);
    assert(buf2);
    lv_display_set_buffers(display, buf1, buf2, BUFFER_SIZE, RENDER_MODE);
    #elif CONFIG_CONNECTION_I2C
    /* 
        I2C config 
        One buffer for mono OLED, render FULL
    */
    size_t draw_buffer_sz = DISP_HOR_RES * DISP_VER_RES / 8 + LVGL_PALETTE_SIZE;  // +8 bytes for monochrome
    buf1 = heap_caps_calloc(1, draw_buffer_sz, MALLOC_CAP_INTERNAL |  MALLOC_CAP_8BIT);
    buf2 = heap_caps_calloc(1, draw_buffer_sz, MALLOC_CAP_INTERNAL |  MALLOC_CAP_8BIT);
    assert(buf1);
    assert(buf2);
    /* Monochromatic */
    lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, RENDER_MODE);
    #endif // CONFIG_CONNECTION
    
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, flush_cb);
    
    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_flush_ready,
    };

    /* Register done callback */
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display);
    /* Timer set in func */
    lvgl_tick_init(); // timer

    set_resolution();
    set_orientation();
    lv_display_set_default(display);    // Set this display as default for UI use

    #ifdef CONFIG_CONNECTION_SPI
    /*
        Drop any theme if exist
        Skip for i2c
    */
    bool is_def = lv_theme_default_is_inited();
    if (is_def) {
        // drop the default theme
        ESP_LOGI(TAG, "Drop the default theme");
        lv_theme_default_deinit();
    }
    #endif // CONNECTION SPI

    // Next: call a task function in display_driver -> display_init function.
    // Each display type will have a separate task setup and graphics
    // Draw different level of graphics at displays
    #ifdef CONFIG_CONNECTION_SPI
    // Now create a task
    ESP_LOGI(TAG, "Create LVGL task");
    // TODO: Make task for SPI display
    xTaskCreatePinnedToCore(lvgl_task_spi_sq_line, "spi display task", 8192, NULL, 9, NULL, tskNO_AFFINITY);
    #elif CONFIG_CONNECTION_I2C
    // Now create a task
    ESP_LOGI(TAG, "Create LVGL task");
    // xTaskCreatePinnedToCore(lvgl_task_i2c, "i2c display task", 8192, NULL, 9, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(lvgl_task_i2c_sq_line, "i2c display task SqLine", 8192, NULL, 9, NULL, tskNO_AFFINITY);
    #endif

    return ESP_OK;
}