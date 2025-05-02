#include <stdio.h>
#include "webserver.h"


const char *lfs_index_path = LFS_MOUNT_POINT"/index.html";
const char *lfs_upload_script_path = LFS_MOUNT_POINT"/upload_script.html";
const char *lfs_icon_path = LFS_MOUNT_POINT"/favicon.ico";


static const char *TAG = "webserver";


void webserver(void) {
    printf(" - Init: webserver empty function call!\n\n");
    ESP_LOGI(TAG, "USERNAME: %s", USERNAME);
    ESP_LOGI(TAG, "PASSWORD: %s", PASSWORD);
    ESP_LOGI(TAG, "BASIC_AUTH: %d", BASIC_AUTH);
    ESP_LOGI(TAG, "SD_MOUNT_POINT new root for webserver: %s", SD_MOUNT_POINT);
    ESP_LOGI(TAG, "LFS_MOUNT_POINT init (fallback) root for webserver: %s", LFS_MOUNT_POINT);
    ESP_LOGI(TAG, "MAX_FILE_SIZE_STR (check html JS too): %s", MAX_FILE_SIZE_STR);
}


esp_err_t start_webserver(void) {
    // 
    webserver();
    return ESP_OK;
}