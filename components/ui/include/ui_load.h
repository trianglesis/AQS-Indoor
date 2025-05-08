#pragma once
#include <string.h>
#include "esp_log.h"

/*
Based on which driver used - load SquareLine Studio graphics for each
See components\ui\CMakeLists.txt
*/
#include "ui.h"

void ui_init_info(void);

/*
Based on which driver used - load SquareLine Studio graphics for each
*/
void load_graphics(void);
