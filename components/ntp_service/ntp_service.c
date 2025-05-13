#include <stdio.h>
#include "ntp_service.h"

static const char *TAG = "time";

bool time_set = false;


void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}


static void obtain_time(void) {   
    // Wifi should be UP already
    // TODO: Skip NTP if WiFi is in AP mode
    if (!time_set) {
        ESP_LOGI(TAG, "Initializing and starting SNTP");
        /*
        * This is the basic default config with one server and starting the service
        */
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
        config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want
        config.smooth_sync = true;
        esp_netif_sntp_init(&config);
        // wait for time to be set
        time_t now = 0;
        struct tm timeinfo = { 0 };
        int retry = 0;
        const int retry_count = 15;
        while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
            ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        }
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    // Set TZ after NTP init
    set_timezone();

}

void time_service_info(void) {
    printf("\n\n- Init:\t\t NTP Service debug info!\n");
    ESP_LOGI(TAG, "SNTP_TIME_SERVER: %s", SNTP_TIME_SERVER);
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        time_set = false;
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
}

void set_timezone(void) {
    setenv("TZ", "EST", 1);
    tzset();
    ESP_LOGI(TAG, "Timezome set to: EST");
}

esp_err_t start_ntp_service(void) {
    time_service_info();
    obtain_time();
    return ESP_OK;
}

