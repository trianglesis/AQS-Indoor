#include <stdio.h>
#include "sensor_co2.h"

static const char *TAG = "sensor-co2";

QueueHandle_t mq_co2;  // Always init early, even empty!

i2c_master_dev_handle_t scd41_handle; // Update as soon as all other 

void sensor_co2_info(void) {
    printf("\n\n- Init:\t\tSensor CO2 SCD4x debug info!\n");
    ESP_LOGI(TAG, "SCD4x SDA_PIN: %d", SCD40_SDA_PIN);
    ESP_LOGI(TAG, "SCD4x SCL_PIN: %d", SCD40_SCL_PIN);
    ESP_LOGI(TAG, "SCD4x ADDRESS: 0x%x", I2C_SCD40_ADDRESS);
    ESP_LOGI(TAG, "SCD4x CO2_MEASUREMENT_FREQ: %d", CO2_MEASUREMENT_FREQ);
    ESP_LOGI(TAG, "SCD4x CO2_LED_UPDATE_FREQ: %d", CO2_LED_UPDATE_FREQ);
    // Check i2c bus
    if (bus_handle != NULL) {
        ESP_LOGI(TAG, "i2c bus is set and not null");
    }

}

static void save_to_db(struct SCD4XSensor* co2_r) {
    char db_name[32];
    snprintf(db_name, sizeof(db_name)-1, "%s/stats.db", DB_ROOT);
    sqlite3 *db;
    sqlite3_initialize();
    int rc = db_open(db_name, &db); // will print "Opened database successfully"
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB INSERT Cannot open database");
    }
    // Save to database
    char table_sql[256];
    snprintf(table_sql, sizeof(table_sql) + sizeof(co2_r) + 1, "INSERT INTO co2_stats VALUES (%f, %f, %d, %d);", co2_r->temperature, co2_r->humidity, co2_r->co2_ppm, co2_r->measure_freq);

    rc = db_exec(db, table_sql);
    if (rc != SQLITE_OK) {
        ESP_LOGE(TAG, "DB INSERT, cannot insert: \n%s\n", table_sql);
    }
    sqlite3_close(db);
}

/*
 Check 
*/
void co2_scd4x_reading(void * pvParameters) {
    bool dataReady;

    // Wait 5 seconds before goint into the loop, sensor should warm up
    vTaskDelay(pdMS_TO_TICKS(5000));
    // Wait TS between cycles real time
    TickType_t last_wake_time  = xTaskGetTickCount();  
    while (1) {
        uint16_t co2Raw;         // ppm
        int32_t t_mili_deg;  // millicelsius
        int32_t humid_mili_percent;     // millipercent
        struct SCD4XSensor co2_r = {};

        // Check if measurements are ready, for 5 sec cycle
        dataReady = scd4x_get_data_ready_status(scd41_handle);
        if (!dataReady) {
            ESP_LOGI(TAG, "CO2 data ready status is not ready! Wait for next check.");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Waiting 1 sec before checking again!
            continue;
        } else {
            // Now read
            scd4x_read_measurement(scd41_handle, &co2Raw, &t_mili_deg, &humid_mili_percent);
            // ESP_LOGI(TAG, "\t\t %d \t\t %ld (raw) \t %ld (raw) ", co2Raw, t_mili_deg, humid_mili_percent);
            // Post conversion from mili
            const int co2Ppm = co2Raw;
            const float t_celsius = t_mili_deg / 1000.0f;
            const float humid_percent = humid_mili_percent / 1000.0f;
            ESP_LOGI(TAG, "CO2:%dppm; Temperature:%.1f; Humidity:%.1f", co2Ppm, t_celsius, humid_percent);
            // Add to queue
            co2_r.co2_ppm = co2Ppm;
            co2_r.temperature = t_celsius;
            co2_r.humidity = humid_percent;
            co2_r.measure_freq = CONFIG_CO2_MEASUREMENT_FREQ;
            xQueueOverwrite(mq_co2, (void *)&co2_r);

            // Update LED
            led_co2_severity(co2_r.co2_ppm);

            // Save to database
            save_to_db(&co2_r);
            xTaskDelayUntil(&last_wake_time, CONFIG_CO2_MEASUREMENT_FREQ);
        } // Data ready
    } // WHILE
}


/*
Always create queue first, at the very beginning.
*/
void create_mq_co2() {
    // Message Queue
    mq_co2 = xQueueGenericCreate(1, sizeof(struct SCD4XSensor), queueQUEUE_TYPE_SET);
    if (!mq_co2) {
        ESP_LOGE(TAG, "queue creation failed");
    }
}

void task_co2() {
    // Task CO2 requires LED
    led_init();
    // Put measurements into the queue
    create_mq_co2();

    xTaskCreate(check_or_create_table, "table-co2_table", 1024*6, (void *)"co2_stats", 5, NULL);

    // Cycle getting measurements
    xTaskCreatePinnedToCore(co2_scd4x_reading, "co2_scd4x_reading", 1024*6, NULL, 4, NULL, tskNO_AFFINITY);
}

/*
Get I2C bus 
Add SCD40 sensor at it, and use device handle

TODO: Save last state to SPI flash: littlefs

TODO: Add SD card read\write option to save states:
- last operation mode
- last power mode
- calibration values
- last measurement or even log

*/
esp_err_t scd40_sensor_init(void) {
    esp_err_t ret;
    sensor_co2_info();  // Debug 

    i2c_device_config_t scd41_device = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_SCD40_ADDRESS,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    // Add device to the bus
    scd41_handle = master_bus_device_add(scd41_device);
    if (scd41_handle == NULL) {
        ESP_LOGI(TAG, "Device handle is NULL! Exit!");
        return ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "Device added! Probe address!");
    }
    // Wait for sensor to wake up, test address and stop measurement after
    ESP_ERROR_CHECK(master_bus_probe_address(I2C_SCD40_ADDRESS, 50)); // Wait 50 ms
    
    // Probably a good idea is to shut the sensor before use it again.
    ret = scd4x_stop_periodic_measurement(scd41_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot send command to device to stop measurements mode!");
        return ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "Stopped measirements now! Can start again after cooldown.");
    }
    
    // Get serial N
    uint16_t serial_number[3] = {0};
    ret = scd4x_get_serial_number(scd41_handle, serial_number, sizeof(serial_number));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot get serial number!");
        return ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "Sensor serial number is: 0x%x 0x%x 0x%x", 
            (int)serial_number[0], 
            (int)serial_number[1], 
            (int)serial_number[2]);
    }
    
    // TODO: Save states to SD Card.
    // TODO: Read previous states from SD Card
    // TODO: If no SD Card - write to SPI flash partition

    // Switch SCD40 to measurement mode
    // It will not respond to most of other commands in this mode, check the datasheet!
    ret = scd4x_start_periodic_measurement(scd41_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start periodic measurement mode!");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Starting the periodic mesurement mode, you can check the next data in 5 seconds since now.");
        // Wait moved into the task init part
    }

    // Create a queue and start task
    task_co2(); 
    return ESP_OK;
}
