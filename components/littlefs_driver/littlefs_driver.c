#include <stdio.h>

#include "littlefs_driver.h"

static const char *TAG = "littlefs";


void littlefs_driver(void)
{
    printf(" - Init: littlefs_driver empty function call!\n");
    ESP_LOGI(TAG, "LFS_MOUNT_POINT: %s", LFS_MOUNT_POINT);
    ESP_LOGI(TAG, "LFS_PARTITION_NAME: %s", LFS_PARTITION_NAME);

}
