#include <stdio.h>
#include "sensor_temp.h"

static const char *TAG = "sensor-bme680";

QueueHandle_t mq_bme680;

bme680_handle_t dev_hdl;


void sensor_temp(void) {
    printf("\n\n- Init:\t\tSensor BME680 debug info!\n");
    ESP_LOGI(TAG, "BME680 SDA_PIN: %d", BME680_SDA_PIN);
    ESP_LOGI(TAG, "BME680 SCL_PIN: %d", BME680_SCL_PIN);
    ESP_LOGI(TAG, "BME680 CONFIG_BME680_I2C_ADDR_0: 0x%x", CONFIG_BME680_I2C_ADDR_0);
    ESP_LOGI(TAG, "BME680 CONFIG_BME680_I2C_ADDR_1: 0x%x", CONFIG_BME680_I2C_ADDR_1);
    ESP_LOGI(TAG, "BME680 MEASUREMENT_FREQ: %d", BME680_MEASUREMENT_FREQ);
    // Check i2c bus
    if (bus_handle != NULL) {
        ESP_LOGI(TAG, "i2c bus is set and not null");
    }
}

void bme680_reading(void * pvParameters) {
    // Wait 5 seconds before goint into the loop, sensor should warm up
    vTaskDelay(pdMS_TO_TICKS(5000));
    // Wait TS between cycles real time
    TickType_t last_wake_time = xTaskGetTickCount();  
    while (1) {
        esp_err_t result;
        struct BMESensor bme680_readings = {};
        bme680_data_t data;

        // 9 profiles?
        for(uint8_t i = 0; i < dev_hdl->dev_config.heater_profile_size; i++) {
            result = bme680_get_data_by_heater_profile(dev_hdl, i, &data);
            if(result != ESP_OK) {
                ESP_LOGE(TAG, "bme680 device read failed (%s)", esp_err_to_name(result));
            }
            // ESP_LOGI(TAG, "Index Air(°C) Dew-Point(°C) Humidity(%%) Pressure(hPa) Gas-Resistance(kΩ) Gas-Range Gas-Valid Gas-Index Heater-Stable IAQ-Score");
            // ESP_LOGI(TAG, "%u\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%u\t%s\t%u\t%s\t%u (%s)",
            //     i,
            //     data.air_temperature,
            //     data.dewpoint_temperature,
            //     data.relative_humidity,
            //     data.barometric_pressure/100,
            //     data.gas_resistance/1000,
            //     data.gas_range,
            //     data.gas_valid ? "yes" : "no",
            //     data.gas_index,
            //     data.heater_stable ? "yes" : "no",
            //     data.iaq_score, bme680_air_quality_to_string(data.iaq_score));
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        
        // Simple?
        // result = bme680_get_data(dev_hdl, &data);
        // if(result != ESP_OK) {
        //     ESP_LOGE(TAG, "bme680 device read failed (%s)", esp_err_to_name(result));
        // }
        // Once per measure
        bme680_readings.temperature = data.air_temperature;
        bme680_readings.humidity = data.relative_humidity;
        bme680_readings.pressure = data.barometric_pressure/100;
        bme680_readings.resistance = data.gas_resistance/1000;
        bme680_readings.air_q_index = data.iaq_score;
        ESP_LOGI(TAG, "t:%.2fC; Humidity:%.2f%%; Pressure:%.2fhpa; Resistance:%.2f; Stable:%s: AQI:%d (%s)",
            data.air_temperature,
            data.relative_humidity,
            data.barometric_pressure/100,
            data.gas_resistance/1000,
            data.heater_stable ? "yes" : "no",
            data.iaq_score, 
            bme680_air_quality_to_string(data.iaq_score)
        );
        
        bme680_readings.measure_freq = BME680_MEASUREMENT_FREQ;
        xQueueOverwrite(mq_bme680, (void *)&bme680_readings);

        // Save to database
        bme680_stats();

        // Actual sleep real time?
        xTaskDelayUntil(&last_wake_time, BME680_MEASUREMENT_FREQ);
    }
}

static inline void print_registers(bme680_handle_t handle) {
    /* configuration registers */
    bme680_control_measurement_register_t ctrl_meas_reg;
    bme680_control_humidity_register_t    ctrl_humi_reg;
    bme680_config_register_t              config_reg;
    bme680_control_gas0_register_t        ctrl_gas0_reg;
    bme680_control_gas1_register_t        ctrl_gas1_reg;

    /* attempt to read control humidity register */
    bme680_get_control_humidity_register(handle, &ctrl_humi_reg);

    /* attempt to read control measurement register */
    bme680_get_control_measurement_register(handle, &ctrl_meas_reg);

    /* attempt to read configuration register */
    bme680_get_configuration_register(handle, &config_reg);

    /* attempt to read control gas 0 register */
    bme680_get_control_gas0_register(handle, &ctrl_gas0_reg);

    /* attempt to read control gas 1 register */
    bme680_get_control_gas1_register(handle, &ctrl_gas1_reg);

    ESP_LOGI(TAG, "Variant Id          (0x%02x): %s", handle->variant_id,uint8_to_binary(handle->variant_id));
    ESP_LOGI(TAG, "Configuration       (0x%02x): %s", config_reg.reg,    uint8_to_binary(config_reg.reg));
    ESP_LOGI(TAG, "Control Measurement (0x%02x): %s", ctrl_meas_reg.reg, uint8_to_binary(ctrl_meas_reg.reg));
    ESP_LOGI(TAG, "Control Humidity    (0x%02x): %s", ctrl_humi_reg.reg, uint8_to_binary(ctrl_humi_reg.reg));
    ESP_LOGI(TAG, "Control Gas 0       (0x%02x): %s", ctrl_gas0_reg.reg, uint8_to_binary(ctrl_gas0_reg.reg));
    ESP_LOGI(TAG, "Control Gas 1       (0x%02x): %s", ctrl_gas1_reg.reg, uint8_to_binary(ctrl_gas1_reg.reg));
}

void create_mq_bme680() {
    // Message Queue
    mq_bme680 = xQueueGenericCreate(1, sizeof(struct BMESensor), queueQUEUE_TYPE_SET);
    if (!mq_bme680) {
        ESP_LOGE(TAG, "queue creation failed");
    }
}

void task_bme680() {
    // Put measurements into the queue
    create_mq_bme680();
    // Start task
    const uint32_t free_before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    xTaskCreatePinnedToCore(bme680_reading, "bme680_reading", 1024*3, NULL, 4, NULL, tskNO_AFFINITY);
    const uint32_t free_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    ssize_t delta = free_after - free_before;
    ESP_LOGI(TAG, "MEMORY for BME680 TASKs\n\tBefore: %"PRIu32" bytes\n\tAfter: %"PRIu32" bytes\n\tDelta: %d\n\n", free_before, free_after, delta);
}

/*
Get I2C bus 
Add sensor at it, and use device handle

TODO: Add SD card read\write option to save states:
- last operation mode
- last power mode
- calibration values
- last measurement or even log

*/
esp_err_t bme680_sensor_init(void) {    
    sensor_temp();  // Debug 
    // initialize i2c device configuration
    bme680_config_t dev_cfg = I2C_BME680_FORCED_CONFIG_DEFAULT;
    // init device
    bme680_init(bus_handle, &dev_cfg, &dev_hdl);
    if (dev_hdl == NULL) {
        ESP_LOGE(TAG, "bme680 handle init failed");
        assert(dev_hdl);
    }
    ESP_ERROR_CHECK(master_bus_probe_address(BME680_I2C_ADDR_1, 50)); // Wait 50 ms
    print_registers(dev_hdl);

    // Create a queue and start task
    task_bme680();
    return ESP_OK;
}


