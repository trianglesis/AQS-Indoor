#pragma once
#include <string.h>
#include "esp_log.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "mbedtls/md5.h"
#include "sdmmc_cmd.h"
#include "esp_vfs.h"
#include "esp_flash.h"
#include "esp_vfs_fat.h"

#include "sqlite_driver.h"

#define MAX_CHAR_SIZE    64


#define SD_GPIO_MOSI                CONFIG_SD_GPIO_MOSI
#define SD_GPIO_SCLK                CONFIG_SD_GPIO_SCLK
#define SD_GPIO_MISO                CONFIG_SD_GPIO_MISO
#define SD_GPIO_CS                  CONFIG_SD_GPIO_CS
#define SD_MOUNT_POINT              CONFIG_SD_MOUNT_POINT
#define FORMAT_IF_MOUNT_FAILED      CONFIG_FORMAT_IF_MOUNT_FAILED

#ifdef CONFIG_FORMAT_AT_MOUNT
#define FORMAT_AT_MOUNT CONFIG_FORMAT_AT_MOUNT
#else
#define FORMAT_AT_MOUNT "No"
#endif

extern uint32_t SDCard_Size;
extern uint32_t Flash_Size;

extern float sd_total;
extern float sd_free;


void card_driver(void);
void card_info(void);
void card_space(void);

esp_err_t card_init(void);

void Flash_Searching(void);