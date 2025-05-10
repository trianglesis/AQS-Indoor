#include <stdio.h>
#include "captive_portal.h"

static const char *TAG = "captive-portal";


void captive_portal(void)
{
    if (USE_CAPTIVE_PORTAL) {
        printf("\n\n- Init:\t\tCaptive Portal debug info!\n");
        ESP_LOGI(TAG, "USE_CAPTIVE_PORTAL: %d", USE_CAPTIVE_PORTAL);
    }
}
