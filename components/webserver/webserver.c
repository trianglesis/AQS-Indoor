#include <stdio.h>
#include "webserver.h"

bool index_exists = false;
const char *index_html_path = NULL;

static const char *TAG = "webserver";


void webserver(void) {
    printf(" - Init: webserver empty function call!\n\n");
    ESP_LOGI(TAG, "USERNAME: %s", USERNAME);
    ESP_LOGI(TAG, "PASSWORD: %s", PASSWORD);
    ESP_LOGI(TAG, "BASIC_AUTH: %d", BASIC_AUTH);
    ESP_LOGI(TAG, "AP_MODE: %d", AP_MODE);
    ESP_LOGI(TAG, "FOUND_WIFI: %d", FOUND_WIFI);
    ESP_LOGI(TAG, "SD_MOUNT_POINT new root for webserver: %s", SD_MOUNT_POINT);
    ESP_LOGI(TAG, "LFS_MOUNT_POINT init (fallback) root for webserver: %s", LFS_MOUNT_POINT);
    ESP_LOGI(TAG, "MAX_FILE_SIZE_STR (check html JS too): %s", MAX_FILE_SIZE_STR);
    // BASIC_AUTH is not yet implemented and may never be, it's just info server...
}

/*
Test path to index.html file at SD Card mount path.
If path is not exist - check index.html at LittleFS mount path.
Return/update var for a valid path.

It checks if index.html file is present at SD Card first.
If not - it will try to read SPI Flash (littleFS) index.html file (from PROJ_DIR/flash_data)
As soon as you upload a new index.html into SD card it will no longer use it from SPI
*/
void check_indexes_locations(void) {
    struct stat st;  
    const char *file_path = NULL;
    file_path = SD_MOUNT_POINT"/index.html";
    // Check is SD path exists, first, Serve LFS if not.
    if (stat(file_path, &st) == 0) {
        index_exists = true;
        index_html_path = file_path;
        ESP_LOGI(TAG, "Web root index.html file exists at SD Card: %d", stat(file_path, &st));
    } else {
        // Serve index html from LittleFS if there is no index at SD Card yet.
        // snprintf(file_path, sizeof(file_path), "%s/index.html", LFS_MOUNT_POINT);
        file_path = LFS_MOUNT_POINT"/index.html";
        if (stat(file_path, &st) == 0) {
            index_exists = true;
            index_html_path = file_path;
            ESP_LOGI(TAG, "Web root index.html file exists at LittleFS but not at SD Card: %d", stat(file_path, &st));
        } else {
            ESP_LOGE(TAG, "Index HTML file not exist at LittleFS partition or SD Card! Cannot serve any page!\n\tDo not start webserver!");
        }
    }
}

/*
Load HTML from VALID and exist path.
Path validation is at the check_indexes_locations()

I've used this read_file as example to understand the proper pointer use.
- https://github.com/DaveGamble/cJSON/blob/acc76239bee01d8e9c858ae2cab296704e52d916/tests/common.h#L47
*/
char* load_index_html_file(const char *filepath) {
    FILE *file = NULL;
    char *content = NULL;
    size_t read_chars = 0;
    
    struct stat info;
    ESP_LOGI(TAG, "Load HTML index...");
    // Test the file existence and get its size in bytes
    if (stat(filepath, &info) == 0) {
        ESP_LOGI(
            TAG, "Index HTML exists: %s; \n\t - Last changed: %s \n\t - File size: %ld bytes", 
            index_html_path, 
            ctime(&info.st_mtime), 
            info.st_size);
    }
    // Read file
    file = fopen(filepath, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Cannot open HTML file for read from path: %s", index_html_path);
    }
    // Allocate memory for file to read into
    content = (char*)malloc((size_t)info.st_size + sizeof(""));
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
    }
    // Read the file
    read_chars = fread(content, sizeof(char), (size_t)info.st_size, file);
    if ((long)read_chars != info.st_size) {
        free(content);
        content = NULL;
    }
    content[read_chars] = '\0';
    // Close now
    if (file != NULL) {
        fclose(file);
    }
    return content;
}

/* 
    Root page if present
    https://stackoverflow.com/a/35325067
*/
esp_err_t root_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Load root handler, start index html page read!");
    // Check again, if the is already present html file at SD card.
    check_indexes_locations();
    // Load html page from validated path
    char *content = load_index_html_file(index_html_path);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Main page root
const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler
};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect...", HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

esp_err_t start_fileserver(void) {
    // static struct file_server_data *server_data = NULL;
    ESP_LOGI(TAG, "Starting file server");
    return ESP_OK;
}

esp_err_t start_webserver(void) {
    // Init
    webserver();
    ESP_LOGI(TAG, "Starting webserver");
    
    // TODO: Detect if we are in AP mode - do not host file server, if STA mode - host file server
    check_indexes_locations();
    if (index_html_path != NULL) {
        ESP_LOGI(TAG, "Index html file found, can start web server now.");
    } else {
        ESP_LOGE(TAG, "Index html file WAS NOT found, skipping web server and file server starting now.\n\nCheck proper mount for LittleFS and SD Card\n\n");
        return ESP_FAIL;
    }

    // Webserver at first
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // always check config LWIP_MAX_SOCKETS = 20
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 10;
    config.max_resp_headers = 10;

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    // Main page and 404
    // Maing page is served from SPI if no same-named index.html was found at SD card!
    httpd_register_uri_handler(server, &root);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);

    // Last, and check if WiFi is STA and ok
    start_fileserver();
    return ESP_OK;
}