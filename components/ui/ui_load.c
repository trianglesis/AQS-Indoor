#include <stdio.h>
#include "ui_load.h"

static const char *TAG = "ui-exported";


void ui_init_info(void) {
    printf(" - Init: SquareLine UI empty function call!\n\n");
    #ifdef CONFIG_CONNECTION_SPI
    ESP_LOGI(TAG, "Put exported UI files in the dir /ui/sq_line_ST7789V3/");
    #elif CONFIG_CONNECTION_I2C
    ESP_LOGI(TAG, "Put exported UI files in the dir /ui/sq_line_SSD1306/");
    #endif // CONFIG_CONNECTION
}

/*
Based on which driver used - load SquareLine Studio graphics for each
*/
void load_graphics(void) {
    ui_init_info(); // Debug
    ui_init(); // NOTE: Always init UI from SquareLine Studio export!
}