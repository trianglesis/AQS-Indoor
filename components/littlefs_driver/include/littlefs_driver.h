#pragma once
#include <string.h>
#include "esp_log.h"


#define LFS_MOUNT_POINT CONFIG_LFS_MOUNT_POINT
#define LFS_PARTITION_NAME CONFIG_LFS_PARTITION_NAME

void littlefs_driver(void);
