#pragma once
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"

#include "sqlite3.h"
#include "sqllib.h"

#define DB_ROOT  CONFIG_SD_MOUNT_POINT

esp_err_t setup_db(void);

void check_or_create_table(void *pvParameters);
void insert_task(void *pvParameters);
void ins_task(void *pvParameters);