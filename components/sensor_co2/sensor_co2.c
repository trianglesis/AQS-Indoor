#include <stdio.h>
#include "sensor_co2.h"

static const char *TAG = "sensor-co2";

QueueHandle_t mq_co2;  // Always init early, even empty!

i2c_master_dev_handle_t scd41_handle; // Update as soon as all other 

void sensor_co2(void) {
    printf(" - Init: sensor_co2 empty function call!\n\n");
    ESP_LOGI(TAG, "SCD40 COMMON_SDA_PIN: %d", COMMON_SDA_PIN);
    ESP_LOGI(TAG, "SCD40 COMMON_SCL_PIN: %d", COMMON_SCL_PIN);
    ESP_LOGI(TAG, "I2C_SCD40_ADDRESS: %x", I2C_SCD40_ADDRESS);
}

/*
 Check 
*/
void co2_scd4x_reading(void * pvParameters) {
    bool dataReady;
    uint16_t co2Raw;         // ppm
    int32_t t_mili_deg;  // millicelsius
    int32_t humid_mili_percent;     // millipercent

    // Wait 5 seconds before goint into the loop, sensor should warm up
    vTaskDelay(pdMS_TO_TICKS(5000));
    TickType_t last_wake_time  = xTaskGetTickCount();  
    while (1) {
        struct SCD4XSensor scd4x_readings = {};
        // Check if measurements are ready, for 5 sec cycle
        dataReady = scd4x_get_data_ready_status(scd41_handle);
        if (!dataReady) {
            ESP_LOGD(TAG, "CO2 data ready status is not ready! Wait for next check.");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Waiting 1 sec before checking again!
            continue;
        } else {
            // Now read
            scd4x_read_measurement(scd41_handle, &co2Raw, &t_mili_deg, &humid_mili_percent);
            ESP_LOGD(TAG, "RAW Measurements ready co2: %d, t: %ld C Humidity: %ld (raw value)", 
                co2Raw, 
                t_mili_deg, 
                humid_mili_percent);
            // Post conversion from mili
            const int co2Ppm = co2Raw;
            const float t_celsius = t_mili_deg / 1000.0f;
            const float humid_percent = humid_mili_percent / 1000.0f;
            ESP_LOGD(TAG, "CO2: %d ppm, Temperature: %.1f C Humidity: %.1f%%\n", 
                co2Ppm, 
                t_celsius, 
                humid_percent);
            // Add to queue
            scd4x_readings.co2_ppm = co2Ppm;
            scd4x_readings.temperature = t_celsius;
            scd4x_readings.humidity = humid_percent;
            xQueueOverwrite(mq_co2, (void *)&scd4x_readings);

            // Waiting before get next measurements, usually not LT 5 sec.
            // vTaskDelay(pdMS_TO_TICKS(CONFIG_CO2_MEASUREMENT_FREQ));

            // Actual sleep real time?
            xTaskDelayUntil(&last_wake_time, CONFIG_CO2_MEASUREMENT_FREQ);
        }

    }
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

    sensor_co2();

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

    return ESP_OK;
}


/*
Led HUE based on CO2 levels as task
    xQueueReceive - destroy the message
    xQueuePeek - read the message, not destroying
*/
void led_co2(void * pvParameters) {
    // Read from the queue
    struct SCD4XSensor scd4x_readings; // data type should be same as queue item type
    const TickType_t xTicksToWait = pdMS_TO_TICKS(CONFIG_CO2_LED_UPDATE_FREQ);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(CONFIG_CO2_LED_UPDATE_FREQ));  // idle between cycles
        xQueuePeek(mq_co2, (void *)&scd4x_readings, xTicksToWait);
        // Update LED colour
        led_co2_severity(scd4x_readings.co2_ppm);
    }
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
    // Put measurements into the queue
    create_mq_co2();
    // Cycle getting measurements
    xTaskCreatePinnedToCore(co2_scd4x_reading, "co2_scd4x_reading", 4096, NULL, 4, NULL, tskNO_AFFINITY);
    // Change LED color based on CO2 severity level
    xTaskCreatePinnedToCore(led_co2, "led_co2", 4096, NULL, 8, NULL, tskNO_AFFINITY);

}