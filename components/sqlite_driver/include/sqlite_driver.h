#pragma once
#include <string.h>
#include "esp_log.h"

#include "sqlite3.h"
#include "sqllib.h"

#include "card_driver.h"


#define DB_ROOT              SD_MOUNT_POINT
extern MessageBufferHandle_t xMessageBufferQuery;

void sqllite_info(void);
void setup_db(void);
void check_or_create_tables(void);