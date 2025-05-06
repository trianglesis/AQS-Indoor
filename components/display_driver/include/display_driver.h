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

#ifdef CONFIG_CONNECTION_SPI
// Setup as DataSheet shows
#define DISP_SPI_SCLK CONFIG_DISP_GPIO_SCLK
#define DISP_SPI_MOSI CONFIG_DISP_GPIO_MOSI
#define DISP_SPI_RST CONFIG_DISP_GPIO_RST
#define DISP_SPI_DC CONFIG_DISP_GPIO_DC
#define DISP_SPI_CS CONFIG_DISP_GPIO_CS
#define DISP_SPI_BL CONFIG_DISP_GPIO_BL
#elif CONFIG_CONNECTION_I2C
#define DISP_I2C_SDA CONFIG_DISP_I2C_SDA
#define DISP_I2C_SCL CONFIG_DISP_I2C_SCL
#define DISP_I2C_ADR CONFIG_DISP_I2C_ADR
#endif


// SPI config
#ifdef CONFIG_CONNECTION_SPI
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_commands.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

#include "lvgl.h"
#include "lvgl_driver.h"            // Also include LVGL setup here
#include "card_driver.h"            // To test if SD card init

/* LCD size */
#define DISP_HOR_RES                172
#define DISP_VER_RES                320
/* LCD settings */
#define DISP_DRAW_BUFF_HEIGHT       50
/* LCD pins */
// From example https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47
#define DISP_SPI_NUM                SPI2_HOST
// 
#define LCD_PIXEL_CLOCK_HZ          (12 * 1000 * 1000)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8
// 
#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_GPIO       EXAMPLE_PIN_NUM_BK_LIGHT
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TEST_DUTY         (4000)
#define LEDC_ResolutionRatio   LEDC_TIMER_13_BIT
#define LEDC_MAX_Duty          ((1 << LEDC_ResolutionRatio) - 1)

// Glob
extern esp_lcd_panel_handle_t panel_handle;
extern esp_lcd_panel_io_handle_t io_handle;
#endif

#ifdef CONFIG_CONNECTION_SPI
// Initialize the LCD backlight, which has been called in the LCD_Init function
void BK_Init(void);                                                         
// Call this function to adjust the brightness of the backlight. The value of the parameter Light ranges from 0 to 100
void BK_Light(uint8_t Light);
// Call this function to initialize the screen (must be called in the main function)
esp_err_t display_init(void);
#endif

void display_driver(void);
