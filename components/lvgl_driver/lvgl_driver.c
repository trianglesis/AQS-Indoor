#include <stdio.h>
#include "lvgl_driver.h"

static const char *TAG = "lvgl";

void lvgl_driver(void)
{
    printf(" - Init: lvgl_driver empty function call!\n");
    ESP_LOGI(TAG, "ROTATE_DISPLAY: %d", ROTATE_DISPLAY);

}
