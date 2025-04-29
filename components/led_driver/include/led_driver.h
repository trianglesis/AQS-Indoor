#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "led_strip.h"

#define LED_STRIP_GPIO_PIN CONFIG_LED_STRIP_GPIO_PIN
#define LED_COLOUR_SATURATION CONFIG_LED_COLOUR_SATURATION
#define LED_COLOUR_VALUE CONFIG_LED_COLOUR_VALUE

#ifdef CONFIG_FMT_RGB
#define COLOUR_COMPONENT LED_STRIP_COLOR_COMPONENT_FMT_RGB
#elif CONFIG_FMT_GRB
#define COLOUR_COMPONENT LED_STRIP_COLOR_COMPONENT_FMT_GRB
#else
#define COLOUR_COMPONENT LED_STRIP_COLOR_COMPONENT_FMT_RGB
#endif

// Do not used
#define LED_STRIP_USE_DMA 0

// LED OPTS
#define TIME_TICK_MS 500

#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically

#define LED_STRIP_MODEL LED_MODEL_WS2812 // WS2812B-0807
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

void led_driver(void);
int random_int_range(int, int);
void led_control_pixel_random(void);
void led_control_hsv_random(void);
void led_control_pixel(led_strip_handle_t strip,
    uint32_t index,
    uint32_t red,
    uint32_t green,
    uint32_t blue);
void led_control_hsv(led_strip_handle_t strip,
    uint32_t index,
    uint16_t hue,
    uint8_t saturation,
    uint8_t value);
void led_co2_severity(int co2_ppm);
void led_init(void);