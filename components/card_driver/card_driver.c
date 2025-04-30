#include <stdio.h>
#include "card_driver.h"

static const char *TAG = "sd-card";

const char* base_path = SD_MOUNT_POINT;

sdmmc_card_t* card;

uint32_t Flash_Size = 0;
uint32_t SDCard_Size = 0;
float sd_total = 0;
float sd_free = 0;

const char *sd_file_hello = SD_MOUNT_POINT"/hello.txt";


void card_driver(void) {
    printf(" - Init: card_driver empty function call!\n\n");
    ESP_LOGI(TAG, "SD_GPIO_MOSI: %d", SD_GPIO_MOSI);
    ESP_LOGI(TAG, "SD_GPIO_SCLK: %d", SD_GPIO_SCLK);
    ESP_LOGI(TAG, "SD_GPIO_MISO: %d", SD_GPIO_MISO);
    ESP_LOGI(TAG, "SD_GPIO_CS: %d", SD_GPIO_CS);
    ESP_LOGI(TAG, "SD_MOUNT_POINT: %s", SD_MOUNT_POINT);
    ESP_LOGI(TAG, "FORMAT_IF_MOUNT_FAILED: %d", FORMAT_IF_MOUNT_FAILED);
    ESP_LOGI(TAG, "ATTENTION FORMAT_AT_MOUNT is active: %s", FORMAT_AT_MOUNT);
}

void card_info(void) {
    // There is wait_fort_idle function can be usefull on device powering off or SD remove
    sdmmc_card_print_info(stdout, card);
    // 3x1024 to only show Gb
    SDCard_Size = ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024 * 1024);
    ESP_LOGI(TAG, "SD Card detected, size: %ld GB", SDCard_Size);
}

void card_space(void) {
    esp_err_t ret;
    // Check space
    uint64_t total = 0, free = 0;
    ret = esp_vfs_fat_info(base_path, &total, &free);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SD Card partition information (%s)", esp_err_to_name(ret));
    }
    else {
        ESP_LOGI(TAG, "SD card Partition size: total: %llu, free: %llu", total, free);
    }
    sd_total = (total / (1024 * 1024 * 1024)); // Convert to Gb
    sd_free = (free / (1024 * 1024 * 1024)); // Convert to Gb
    ESP_LOGI(TAG, "SD Card size total/free: %.2f/%.2f GB", sd_total, sd_free);
}

// Leave it as in example
esp_err_t s_example_write_file(const char *path, char *data) {
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");
    // Create a few dirs
    mkdir("log", 0755);
    mkdir("upload", 0755);
    mkdir("download", 0755);
    mkdir("test", 0755);
    return ESP_OK;
}

// Based on two different examples
esp_err_t sd_card_file_sum_test(void) {
    ESP_LOGI(TAG, "Computing hello.txt MD5 hash and test reading");
    FILE* f = fopen(sd_file_hello, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open hello.txt");
    }
    // Read file and compute the digest chunk by chunk
    #define MD5_MAX_LEN 16

    char buf[64];
    mbedtls_md5_context ctx;
    unsigned char digest[MD5_MAX_LEN];

    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);

    size_t read;

    do {
        read = fread((void*) buf, 1, sizeof(buf), f);
        mbedtls_md5_update(&ctx, (unsigned const char*) buf, read);
    } while(read == sizeof(buf));

    mbedtls_md5_finish(&ctx, digest);

    // Create a string of the digest
    char digest_str[MD5_MAX_LEN * 2];

    for (int i = 0; i < MD5_MAX_LEN; i++) {
        sprintf(&digest_str[i * 2], "%02x", (unsigned int)digest[i]);
    }

    // For reference, MD5 should be d25b9ac261c79341e71548ddc7101d24
    ESP_LOGI(TAG, "Computed MD5 hash of hello.txt: %s", digest_str);

    fclose(f);
    return ESP_OK;
}

// Based on two different examples
esp_err_t sd_card_file_read_test(void) {
    ESP_LOGI(TAG, "Reading hello.txt");
    // Open for reading hello.txt
    FILE* f = fopen(sd_file_hello, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open hello.txt");
    }
    char buf[64];
    memset(buf, 0, sizeof(buf));
    fread(buf, 1, sizeof(buf), f);
    fclose(f);
    // Display the read contents from the file
    ESP_LOGI(TAG, "Read from hello.txt: %s", buf);
    return ESP_OK;

}

esp_err_t sd_card_file_write_test(void) {
    ESP_LOGI(TAG, "Opening file %s", sd_file_hello);
    FILE *f = fopen(sd_file_hello, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "Test file for SD card init");
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}


/*

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html#_CPPv426esp_vfs_fat_mount_config_t
https://github.com/espressif/esp-idf/blob/4c2820d377d1375e787bcef612f0c32c1427d183/examples/protocols/http_server/file_serving/main/mount.c

*/
esp_err_t card_init(void) {
    esp_err_t ret;

    card_driver();
    ESP_LOGI(TAG, "Initializing SD card");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 10,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Using SPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_GPIO_MOSI,
        .miso_io_num = SD_GPIO_MISO,
        .sclk_io_num = SD_GPIO_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4000,  // Use LVGL buffer size by default BUFFER_SIZE or set a separate one?
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    } else {        
        ESP_LOGI(TAG, "Initialized SPI bus as SD Card init, and skip this step for LCD!");
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_GPIO_CS;
    slot_config.host_id = host.slot;
    ret = esp_vfs_fat_sdspi_mount(base_path, &host, &slot_config, &mount_config, &card);

    // Check errors
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. Set FORMAT_IF_MOUNT_FAILED in header file.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }

        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Show info
    card_info();
    // Space total and free
    card_space();
    // Test
    sd_card_file_write_test();
    sd_card_file_read_test();
    sd_card_file_sum_test();

    return ESP_OK;
}

// SPI, not SD
void Flash_Searching(void) {
    if(esp_flash_get_physical_size(NULL, &Flash_Size) == ESP_OK) {
        Flash_Size = Flash_Size / (uint32_t)(1024 * 1024);
        printf("Flash size: %ld MB\n", Flash_Size);
    } else {
        printf("Get flash size failed\n");
    }
}