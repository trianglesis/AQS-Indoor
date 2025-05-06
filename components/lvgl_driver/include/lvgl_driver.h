#pragma once
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
// Use display
#include "display_driver.h"

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
