#pragma once
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
// Use display, we need panel_handle and io_handle
#include "display_driver.h"

// Check default lvgl conf
#ifdef CONFIG_LV_CONF_SKIP
#else
// CUSTOM configured. Do not use now, use Kconfig
#include "lvgl.h"
#endif

#define ROTATE_DISPLAY CONFIG_ROTATE_DISPLAY
// Rotate 90deg and compensate buffer change
// TODO: Move it to the main file for better visibility
#define Offset_X 0 // 0 IF NOT ROTATED 270deg
#define Offset_Y 34  // 34 IF ROTATED 270deg
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