#include <stdio.h>
#include "card_driver.h"

static const char *TAG = "sd-card";


void card_driver(void)
{
    printf(" - Init: card_driver empty function call!\n");
    ESP_LOGI(TAG, "SD_GPIO_MOSI: %d", SD_GPIO_MOSI);
    ESP_LOGI(TAG, "SD_GPIO_SCLK: %d", SD_GPIO_SCLK);
    ESP_LOGI(TAG, "SD_GPIO_MISO: %d", SD_GPIO_MISO);
    ESP_LOGI(TAG, "SD_GPIO_CS: %d", SD_GPIO_CS);
    ESP_LOGI(TAG, "SD_MOUNT_POINT: %d", SD_MOUNT_POINT);
    ESP_LOGI(TAG, "FORMAT_IF_MOUNT_FAILED: %d", FORMAT_IF_MOUNT_FAILED);
    ESP_LOGI(TAG, "ATTENTION FORMAT_AT_MOUNT is active: %s", FORMAT_AT_MOUNT);
}
