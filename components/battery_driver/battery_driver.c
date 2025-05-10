#include <stdio.h>
#include "battery_driver.h"

static const char *TAG = "adc-battery";

QueueHandle_t mq_batt;

static int adc_raw[2][10];
static int voltage[2][10];

typedef struct adc_task_parameters *adc_task_param_t;

struct adc_task_parameters {
    adc_oneshot_unit_handle_t adc1_handle;
    bool do_calibration1_chan0;
    adc_cali_handle_t adc1_cali_chan0_handle;
};

void battery_driver_info(void) {
    printf("\n\n- Init:\t\tBattery debug info!\n");
    ESP_LOGI(TAG, "ADC_MEASUREMENT_FREQ_MINUTES: %d", ADC_MEASUREMENT_FREQ_MINUTES);
    ESP_LOGI(TAG, "ADC_MEASUREMENT_LOOP_COUNT: %d", ADC_MEASUREMENT_LOOP_COUNT);
    ESP_LOGI(TAG, "ADC_ATTEN: %d", ADC_ATTEN);
    ESP_LOGI(TAG, "ADC_PIN: %d", ADC_PIN);
    ESP_LOGI(TAG, "ADC1_CHAN0: %d", ADC1_CHAN0);
}

void battery_measure_task(void *pvParameters) {

    adc_task_param_t param = pvParameters;
    int max_perc = 100;
    int min_perc = 1;
    int max_volt = BATTERY_CHARGED_VOLTAGE;  // Fully charged, under esp32 load
    int min_volt = BATTERY_DISCHARGED_VOLTAGE;  // Fully discharged
    uint64_t measure_freq = (uint64_t)ADC_MEASUREMENT_FREQ_MINUTES * 60 * 1000;
    int loop_count = ADC_MEASUREMENT_LOOP_COUNT;

    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait
    TickType_t last_wake_time  = xTaskGetTickCount();  
    while (1) {
        struct BattSensor battery_readings = {};
        battery_readings.max_masured_voltage = 0;
        // Loop for 3-5 measurements and choose one MAX
        for (int i = 0; i < loop_count; ++i) {
            ESP_ERROR_CHECK(adc_oneshot_read(param->adc1_handle, ADC1_CHAN0, &adc_raw[0][0]));
            // ESP_LOGI(TAG, "Measure: %d -- ADC%d Channel[%d] Raw Data: %d", i, ADC_UNIT_1 + 1, ADC1_CHAN0, adc_raw[0][0]);
            if (param->do_calibration1_chan0) {
                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(param->adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
                // ESP_LOGI(TAG, "Measure: %d -- ADC%d Channel[%d] Cali Voltage: %d mV", i, ADC_UNIT_1 + 1, ADC1_CHAN0, voltage[0][0]);
            }
            if (battery_readings.max_masured_voltage < voltage[0][0]) {
                battery_readings.max_masured_voltage = voltage[0][0];
                battery_readings.adc_raw = adc_raw[0][0];
                battery_readings.voltage = battery_readings.max_masured_voltage / 1000;
                battery_readings.voltage_m = battery_readings.max_masured_voltage;
            }
            ESP_LOGI(TAG, "Try: %d; measured voltage: %d mV; max measured during this cycle: %d mV", i, voltage[0][0], battery_readings.max_masured_voltage);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        // Linear interpolation
        float batteryLevel = max_perc + (((battery_readings.voltage_m - max_volt) * (min_perc -  max_perc)) / (min_volt - max_volt));
        battery_readings.percentage = batteryLevel;
        ESP_LOGI(TAG, "RAW: %d; Cali: V:%d; Converted V %d; Battery percentage: %d", battery_readings.adc_raw, battery_readings.voltage_m, battery_readings.voltage_m, battery_readings.percentage);

        xQueueOverwrite(mq_batt, (void *)&battery_readings);
        xTaskDelayUntil(&last_wake_time, measure_freq);
    } // WHILE
}

/*
 Use default ESP32C6 ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
*/
static bool adc_calibr_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calibr_deinit(adc_cali_handle_t handle) {
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
}

static void tear_down(bool do_calibration1_chan0, adc_oneshot_unit_handle_t adc1_handle, adc_cali_handle_t adc1_cali_chan0_handle) {
    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        adc_calibr_deinit(adc1_cali_chan0_handle);
    }
}

/*
Always create queue first, at the very beginning.
*/
void create_mq_battery() {
    // Message Queue
    mq_batt = xQueueGenericCreate(1, sizeof(struct BattSensor), queueQUEUE_TYPE_SET);
    if (!mq_batt) {
        ESP_LOGE(TAG, "queue creation failed");
    }
}

/*

    https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c6/api-reference/peripherals/adc_oneshot.html
*/
esp_err_t battery_one_shot_init(void) {
    battery_driver_info();  // Debug
    create_mq_battery(); // Queue always
    
    // Unit
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // Channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN0, &config));

    // Calibration
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = adc_calibr_init(ADC_UNIT_1, ADC1_CHAN0, ADC_ATTEN, &adc1_cali_chan0_handle);

    // Start task
    adc_task_param_t handle = calloc(1, sizeof(struct adc_task_parameters) + sizeof(adc1_handle) + sizeof(do_calibration1_chan0) + sizeof(adc1_cali_chan0_handle));

    handle->adc1_handle = adc1_handle;
    handle->do_calibration1_chan0 = do_calibration1_chan0;
    handle->adc1_cali_chan0_handle = adc1_cali_chan0_handle;

    xTaskCreatePinnedToCore(battery_measure_task, "adc-batt", 4096, handle, 4, NULL, tskNO_AFFINITY);

    return ESP_OK;
}
