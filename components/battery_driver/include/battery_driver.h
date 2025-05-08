
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


#define ADC_PIN                 CONFIG_ADC_PIN
#define ADC1_CHAN0              ADC_CHANNEL_0
#define ADC_MEASUREMENT_FREQ    CONFIG_ADC_MEASUREMENT_FREQ
#define ADC_ATTEN               ADC_ATTEN_DB_12

extern QueueHandle_t mq_batt;

struct BattSensor {
    int adc_raw;
    int voltage;
};


esp_err_t battery_one_shot_init(void);