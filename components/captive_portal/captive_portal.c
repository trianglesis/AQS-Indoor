#include <stdio.h>
#include "captive_portal.h"

static const char *TAG = "captive-portal";


void captive_portal(void)
{
    if (USE_CAPTIVE_PORTAL) {
        printf("\n\n\t\t\t - Init: Captive Portal debug info!");
        ESP_LOGI(TAG, "USE_CAPTIVE_PORTAL: %d", USE_CAPTIVE_PORTAL);
    }
}
