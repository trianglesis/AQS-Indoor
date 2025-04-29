#include <stdio.h>
#include "display_driver.h"

static const char *TAG = "oled-display";


void display_driver(void)
{
    printf(" - Init: display_driver empty function call!\n");
    ESP_LOGI(TAG, "DISP_GPIO_SCLK: %d", DISP_GPIO_SCLK);
    ESP_LOGI(TAG, "DISP_GPIO_MOSI: %d", DISP_GPIO_MOSI);
    ESP_LOGI(TAG, "DISP_GPIO_RST: %d", DISP_GPIO_RST);
    ESP_LOGI(TAG, "DISP_GPIO_DC: %d", DISP_GPIO_DC);
    ESP_LOGI(TAG, "DISP_GPIO_CS: %d", DISP_GPIO_CS);
    ESP_LOGI(TAG, "DISP_GPIO_BL: %d", DISP_GPIO_BL);

}
