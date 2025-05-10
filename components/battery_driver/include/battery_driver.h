
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "esp_log.h"
#include "esp_check.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


#define ADC_PIN                             CONFIG_ADC_PIN
#define ADC1_CHAN0                          ADC_CHANNEL_0
#define ADC_ATTEN                           ADC_ATTEN_DB_12
// Change to minutes:                       1 minute X 60 seconds X 1000 miliseconds
#define ADC_MEASUREMENT_FREQ_MINUTES        CONFIG_ADC_MEASUREMENT_FREQ_MINUTES
#define ADC_MEASUREMENT_LOOP_COUNT          CONFIG_ADC_MEASUREMENT_LOOP_COUNT
#define BATTERY_CHARGED_VOLTAGE             CONFIG_BATTERY_CHARGED_VOLTAGE
#define BATTERY_DISCHARGED_VOLTAGE          CONFIG_BATTERY_DISCHARGED_VOLTAGE


extern QueueHandle_t mq_batt;

struct BattSensor {
    int adc_raw;
    int voltage;
    int voltage_m;
    int percentage;
    int max_masured_voltage;
};


esp_err_t battery_one_shot_init(void);