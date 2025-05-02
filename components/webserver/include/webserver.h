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


// Default Webserver root dir - here upload files: SD Card
#define WEBSERVER_ROOT              SD_MOUNT_POINT
// Upload INIT server root dir - do not upload files here, but use as initial page for upload
#define FILESERVER_INIT_ROOT        LFS_MOUNT_POINT

// AP mode = no fileserver, STA mode and home wifi = file server
#define AP_MODE                     wifi_ap_mode
#define FOUND_WIFI                  found_wifi

// File server opt
/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (500*1024) // 500 KB
#define MAX_FILE_SIZE_STR "500KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

// Server upload files capability
#define QUERY_KEY_MAX_LEN  (64)


void webserver(void);

esp_err_t start_webserver(void);