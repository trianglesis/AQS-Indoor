#include "wifi.h"

static const char *TAG = "wifi-setup";


void wifi(void)
{
    printf(" - Init: wifi empty function call!\n");
    ESP_LOGI(TAG, "WIFI_SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "WIFI_PASS: %s", WIFI_PASS);
    ESP_LOGI(TAG, "WIFI_CHANNEL: %d", WIFI_CHANNEL);
    ESP_LOGI(TAG, "MAX_STA_CONN: %d", MAX_STA_CONN);
}
