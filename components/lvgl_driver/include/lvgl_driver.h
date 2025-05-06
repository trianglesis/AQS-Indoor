#pragma once
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui.h"
// Use display, we need panel_handle and io_handle
#include "display_driver.h"

// Check default lvgl conf
#ifdef CONFIG_LV_CONF_SKIP
#else
// CUSTOM configured. Do not use now, use Kconfig
#include "lvgl.h"
#endif

// Rotate and compensate buffer change
#ifdef CONFIG_CONNECTION_SPI
#define CONNECTION_I2C          0
#define CONNECTION_SPI          1
// Use offset only for Waveshare
#ifdef CONFIG_ROTATE_0
#define ROTATE_DEGREE           0
#define Offset_X                34
#define Offset_Y                0
#elif CONFIG_ROTATE_90
#define ROTATE_DEGREE           90
#define Offset_X                0
#define Offset_Y                34
#elif CONFIG_ROTATE_180
#define ROTATE_DEGREE           180
#define Offset_X                34
#define Offset_Y                0
#elif CONFIG_ROTATE_270
#define ROTATE_DEGREE           270
#define Offset_X                0
#define Offset_Y                34
#else
#endif // CONFIG_ROTATE
#elif CONFIG_CONNECTION_I2C
#define CONNECTION_SPI          0
#define CONNECTION_I2C          1
// Use offset only for I2C
#ifdef CONFIG_ROTATE_0
#define ROTATE_DEGREE           0
#define Offset_X                0
#define Offset_Y                0
#elif CONFIG_ROTATE_90
#define ROTATE_DEGREE           90
#define Offset_X                0
#define Offset_Y                0
#elif CONFIG_ROTATE_180
#define ROTATE_DEGREE           180
#define Offset_X                0
#define Offset_Y                0
#elif CONFIG_ROTATE_270
#define ROTATE_DEGREE           270
#define Offset_X                0
#define Offset_Y                0
#else
#define ROTATE_DEGREE           0
#define Offset_X                0
#define Offset_Y                0
#endif // CONFIG_ROTATE
#endif // CONFIG_CONNECTION_


// Save display when init and use it all over the project
extern lv_disp_t *display; 
// 
#define LVGL_TICK_PERIOD_MS    2
// Display buffer use
#define BUFFER_SIZE            (DISP_HOR_RES * DISP_VER_RES * 2 / 10)

void lvgl_driver(void);
bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
void lvgl_tick_increment(void *arg);
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
void set_resolution(lv_display_t* disp);
esp_err_t lvgl_init(void);

#ifdef CONFIG_CONNECTION_SPI
/*
TODO Add extra graphics similarly to:
- https://github.com/trianglesis/Air_Quality_station/blob/171f9959632b4a9d2c10f1fab73dca86c0ced4c2/main/main.c#L53

As separate function with task
*/
esp_err_t graphics_spi_draw(void);

#elif CONFIG_CONNECTION_I2C
/*
Included simple text and icos for i2c display
*/
esp_err_t graphics_i2c_draw(void);

#endif // CONFIG_CONNECTION_