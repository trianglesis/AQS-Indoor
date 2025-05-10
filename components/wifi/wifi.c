#include "wifi.h"

int connected_users = 0;
bool wifi_ap_mode = false;
bool found_wifi = false;
char ip_string[128];

static int s_retry_num = 0;

static const char *TAG = "wifi-setup";

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;

/* Default wifi AP and STA */
static esp_netif_t *esp_netif_ap = NULL;
static esp_netif_t *esp_netif_sta = NULL;

/* Config for wifi AP an STA */
static wifi_config_t wifi_ap_config = { 0 };
static wifi_config_t wifi_sta_config = { 0 };


void wifi_debug(void) {
    printf("\n\n\t\t\t - Init: WiFi Driver debug info!");
    ESP_LOGI(TAG, "USE_WIFI_WEB: %d", USE_WIFI_WEB);
    ESP_LOGI(TAG, "WIFI_SSID: %s", WIFI_SSID);
    ESP_LOGI(TAG, "WIFI_PASS: %s", WIFI_PASS);
    ESP_LOGI(TAG, "WIFI_CHANNEL: %d", WIFI_CHANNEL);
    ESP_LOGI(TAG, "MAX_STA_CONN: %d", MAX_STA_CONN);
    ESP_LOGI(TAG, "MAX_TRANSMIT_POWER: %d", MAX_TRANSMIT_POWER);
    ESP_LOGI(TAG, "connected_users: %d", connected_users);
    ESP_LOGI(TAG, "ip_string: %s", ip_string);
}

/*
Both AP and STA

    TODO: Check if STA network is no longer available and switch to AP
    
*/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Init WiFI event handler");
    // When AP connected\disconnected
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        // Always increment on connection
        connected_users++;
        ESP_LOGI(TAG, "Connected users update: %d", connected_users);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
        // If 1 or less:
        if (connected_users <= 1 ) {
            connected_users = 0;
        // If 1 or more
        } else if (connected_users >=1 ) {
            connected_users--;
        } else {
            ESP_LOGI(TAG, "Connected users unusual: %d", connected_users);
        }
        ESP_LOGI(TAG, "Connected users left: %d", connected_users);
        // WiFI Station connect to and got IP:
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Station started");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        connected_users = 0;    // No users count when local WiFI is ok
        wifi_ap_mode = false;   // Not an AP mode
        found_wifi = true;      // Show is local WiFI icon

        // Making STR from ipv4
        char *res = NULL;
        res = inet_ntoa_r(event->ip_info.ip, ip_string, sizeof(ip_string) - 1);
        if (!res) {
            ip_string[0] = '\0'; // Returns empty string if conversion didn't succeed
        }
        ESP_LOGI(TAG, "Got IP address_str: %s", ip_string);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Initialize soft AP */
void wifi_init_softap(void) {
    ESP_LOGI(TAG, "Init WiFI AP mode options");
    esp_netif_ap = esp_netif_create_default_wifi_ap();
    
    strlcpy((char *) wifi_ap_config.sta.ssid, WIFI_SSID, sizeof(wifi_ap_config.sta.ssid));
    if (WIFI_PASS) {
        strncpy((char *) wifi_ap_config.ap.password, WIFI_PASS, sizeof(wifi_ap_config.ap.password));
    }

    /*
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    */
    wifi_ap_config.ap.ssid_len = strlen(WIFI_SSID);
    wifi_ap_config.ap.channel = WIFI_CHANNEL;
    wifi_ap_config.ap.max_connection = MAX_STA_CONN;
    wifi_ap_config.ap.pmf_cfg.required = false;

    // Open
    if (strlen(WIFI_PASS) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_LOGI(TAG, "Wifi Soft AP setup finished. SSID:%s password:%s channel:%d", WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);
    wifi_ap_mode = true;  // Initial AP mode, if connected to WiFI found_wifi will be true
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* Initialize wifi station: JOIN WiFi network */
void wifi_init_sta(void) {
    ESP_LOGI(TAG, "Init WiFI Station mode options");
    esp_netif_sta = esp_netif_create_default_wifi_sta();

    strlcpy((char *) wifi_sta_config.sta.ssid, SSID, sizeof(wifi_sta_config.sta.ssid));
    if (PWD) {
        strncpy((char *) wifi_sta_config.sta.password, PWD, sizeof(wifi_sta_config.sta.password));
    }

    wifi_sta_config.sta.threshold.authmode = AUTHMODE;
    wifi_sta_config.sta.scan_method = SCAN_METHOD;
    wifi_sta_config.sta.failure_retry_cnt = ESP_MAXIMUM_RETRY;
    wifi_sta_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    // wifi_sta_config.sta.sort_method = SORT_METHOD,
    // wifi_sta_config.sta.threshold.rssi = RSSI,
    // wifi_sta_config.sta.threshold.rssi_5g_adjustment = RSSI_5G_ADJUSTMENT,

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );
    ESP_LOGI(TAG, "Wifi Station mode setup finished.");
    vTaskDelay(pdMS_TO_TICKS(10));
}

/*
* Use DNS flag
*/
void softap_set_dns_addr(esp_netif_t *esp_netif_ap, esp_netif_t *esp_netif_sta) {
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN, &dns);
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
}

/*
* Wait until either the connection is established (WIFI_CONNECTED_BIT) or
* connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
* The bits are set by event_handler() (see above)
*/
esp_err_t wifi_establishing(void) {
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);
    
    /* xEventGroupWaitBits() returns the bits before the call returned,
     * hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", SSID, PWD);
        softap_set_dns_addr(esp_netif_ap, esp_netif_sta);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", SSID, PWD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
    
    /* Set sta as the default interface */
    esp_netif_set_default_netif(esp_netif_sta);
    /* Enable napt on the AP netif */
    if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK) {
        ESP_LOGE(TAG, "NAPT not enabled on the netif: %p", esp_netif_ap);
    }
    return ESP_OK;
}

void nvs_load(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Init ok.");

}

void event_group(void) {
    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_LOGI(TAG, "Event group added.");
}

/*
* Init Wifi and setup one of two modes.
* 
*/
esp_err_t wifi_setup(void) {

    #ifdef CONFIG_USE_WIFI_WEB
    if(!USE_WIFI_WEB) {
        ESP_LOGI(TAG, "SKIPPED: Wifi, Webserver and Captive portal!");
    } else {
        esp_err_t ret;
        wifi_debug();             // 1

        ESP_LOGI(TAG, "Starting Wifi, init MVS, Events");
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Secure storage
        nvs_load();
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Initialize event group */
        event_group();
        vTaskDelay(pdMS_TO_TICKS(10));

        /*Initialize WiFi */
        ESP_LOGI(TAG, "Init WiFi config default.");
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        
        ESP_LOGI(TAG, "WiFi Set mode APP-STA.");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

        /* Initialize AP */
        wifi_init_softap();

        /* Initialize STA */
        wifi_init_sta();

        /* Start WiFi */
        ESP_LOGI(TAG, "Start Init WiFi...");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_ERROR_CHECK(esp_wifi_start());

        /* Now check known network OR start AP if None */
        ret = wifi_establishing();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot setup Wifi any mode!");
        }

        // Start webserver here after wifi init
        ESP_ERROR_CHECK(start_webserver());
        captive_portal();   // 2
    }
    #else
    ESP_LOGI(TAG, "SKIPPED: Wifi, Webserver and Captive portal!");
    #endif

    return ESP_OK;
}