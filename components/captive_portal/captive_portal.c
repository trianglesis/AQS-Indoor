#include <stdio.h>
#include "captive_portal.h"

static const char *TAG = "captive-portal";


void captive_portal(void)
{
    printf(" - Init: captive_portal empty function call!\n\n");
    if (USE_CAPTIVE_PORTAL) {
        ESP_LOGI(TAG, "USE_CAPTIVE_PORTAL: %d", USE_CAPTIVE_PORTAL);
    }
}
