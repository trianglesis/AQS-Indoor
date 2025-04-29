#include <stdio.h>
#include "webserver.h"

static const char *TAG = "webserver";

void webserver(void)
{
    printf(" - Init: webserver empty function call!\n\n");
    ESP_LOGI(TAG, "USERNAME: %s", USERNAME);
    ESP_LOGI(TAG, "PASSWORD: %s", PASSWORD);
    ESP_LOGI(TAG, "BASIC_AUTH: %d", BASIC_AUTH);
}
