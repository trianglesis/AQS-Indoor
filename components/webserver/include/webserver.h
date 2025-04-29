#pragma once
#include <string.h>
#include "esp_log.h"

#define USERNAME CONFIG_USERNAME
#define PASSWORD CONFIG_PASSWORD
#define BASIC_AUTH CONFIG_BASIC_AUTH

void webserver(void);
