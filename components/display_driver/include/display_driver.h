#pragma once
#include <string.h>
#include "esp_log.h"

#define DISP_GPIO_SCLK CONFIG_DISP_GPIO_SCLK
#define DISP_GPIO_MOSI CONFIG_DISP_GPIO_MOSI
#define DISP_GPIO_RST CONFIG_DISP_GPIO_RST
#define DISP_GPIO_DC CONFIG_DISP_GPIO_DC
#define DISP_GPIO_CS CONFIG_DISP_GPIO_CS
#define DISP_GPIO_BL CONFIG_DISP_GPIO_BL

void display_driver(void);
