#pragma once
#include <string.h>
#include <stdio.h>
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_timer.h"
// Displays common
#include "esp_lcd_panel_io.h"
// My
#include "lvgl_driver.h"            // LVGL required for most displays

// Common handles for any display
extern esp_lcd_panel_handle_t       panel_handle;
extern esp_lcd_panel_io_handle_t    io_handle;

#ifdef CONFIG_LV_CONF_SKIP
#else
// CUSTOM configured. Do not use now, use Kconfig
#include "lvgl.h"
#endif

#ifdef CONFIG_CONNECTION_SPI
/*
SPI Config. 
Example: Waveshare ESP32 C6 LCD
*/
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_commands.h"
// My
#include "card_driver.h"            // To test if SD card init

// Setup as DataSheet shows
#define DISP_SPI_SCLK               CONFIG_DISP_GPIO_SCLK
#define DISP_SPI_MOSI               CONFIG_DISP_GPIO_MOSI
#define DISP_SPI_RST                CONFIG_DISP_GPIO_RST
#define DISP_SPI_DC                 CONFIG_DISP_GPIO_DC
#define DISP_SPI_CS                 CONFIG_DISP_GPIO_CS
#define DISP_SPI_BL                 CONFIG_DISP_GPIO_BL

/* LCD size */
#define DISP_HOR_RES                172
#define DISP_VER_RES                320
/* LCD settings */
#define DISP_DRAW_BUFF_HEIGHT       50
/* LCD pins */
// From example https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47
#define DISP_SPI_NUM                SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ          (12 * 1000 * 1000)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS                8
#define LCD_PARAM_BITS              8
#define LEDC_HS_TIMER               LEDC_TIMER_0
#define LEDC_LS_MODE                LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_GPIO            EXAMPLE_PIN_NUM_BK_LIGHT
#define LEDC_HS_CH0_CHANNEL         LEDC_CHANNEL_0
#define LEDC_TEST_DUTY              (4000)
#define LEDC_ResolutionRatio        LEDC_TIMER_13_BIT
#define LEDC_MAX_Duty               ((1 << LEDC_ResolutionRatio) - 1)

#elif CONFIG_CONNECTION_I2C
/*
I2C Config
*/
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
 // My implementation of most common functions
#include "i2c_driver.h"
// If needed
#include "driver/i2c_master.h"

// Pins
#define DISP_I2C_SDA                CONFIG_COMMON_SDA_PIN
#define DISP_I2C_SCL                CONFIG_COMMON_SCL_PIN
#define DISP_I2C_ADR                CONFIG_DISP_I2C_ADR

#define DISP_I2C_RST                -1
#define LCD_PIXEL_CLOCK_HZ          (400 * 1000)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS                8
#define LCD_PARAM_BITS              8

/* LCD size SSD1306 */
#define DISP_HOR_RES                128
#define DISP_VER_RES                64
#endif

// Common for all
void display_driver(void);
esp_err_t display_init(void);

// SPI Only
#ifdef CONFIG_CONNECTION_SPI
void BK_Init(void);
void BK_Light(uint8_t Light);
esp_err_t display_spi_init(void);
#elif CONFIG_CONNECTION_I2C
esp_err_t display_i2c_init(void);
#endif

