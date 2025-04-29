#include <stdio.h>
#include "ui.h"

static const char *TAG = "ui-exported";


void ui_init_fake(void)
{
    printf(" - Init: SquareLine UI empty function call!\n");
    // Put the init of UI here.
    // Put exported UI files in the dir /exported/
    ESP_LOGI(TAG, "Put exported UI files in the dir /exported/");

}
