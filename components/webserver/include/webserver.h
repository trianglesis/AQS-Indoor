#pragma once
#include <string.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"

// My
#include "wifi.h"
#include "card_driver.h"
#include "littlefs_driver.h"

#define USERNAME CONFIG_USERNAME
#define PASSWORD CONFIG_PASSWORD
#define BASIC_AUTH CONFIG_BASIC_AUTH


// Init root - SPI flash, upload new index.html at SD root dir to change it
#define WEBSERVER_INIT_ROOT         LFS_MOUNT_POINT
#define WEBSERVER_ROOT              SD_MOUNT_POINT
// File server can only use SD card.
#define FILESERVER_ROOT             SD_MOUNT_POINT
#define SD_CARD_FREE                sd_free
#define SD_CARD_TOTAL               sd_total

// AP mode = no fileserver, STA mode and home wifi = file server
#define AP_MODE                     wifi_ap_mode
#define FOUND_WIFI                  found_wifi

// File server opt
/* 
VFS does not impose any limit on total file path length, but it does limit the FS path prefix to ESP_VFS_PATH_MAX characters. Individual FS drivers may have their own filename length limitations.
Also limit to webserver max uri > 
*/
#define FILE_PATH_MAX               128

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE               (1024*1024) // 1024 KB
#define MAX_FILE_SIZE_STR           "1024KB"
/* Scratch buffer size */
#define SCRATCH_BUFSIZE              8192
// Server upload files capability
#define QUERY_KEY_MAX_LEN           (64)

// Server upload files capability
#define UPLOAD_HTML_PATH            "/littlefs/upload_script.html"
#define UPLOAD_FAV_PATH             "/littlefs/favicon.ico"


/*
    Later use at AP mode?
*/
#if BASIC_AUTH

#define HTTPD_401 "401 UNAUTHORIZED"  /*!< HTTP Response 401 */
#define USERNAME "admin"
#define PASSWORD "can_upload_1!"

typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

#endif

/* Check before upload files with 4chars+ extensions */
#ifdef FATFS_LONG_FILENAMES
#define LONG_FILENAMES 0
#else
#define LONG_FILENAMES 1
#endif


void webserver(void);

esp_err_t start_webserver(void);