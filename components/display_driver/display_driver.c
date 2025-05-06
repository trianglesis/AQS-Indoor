#include "display_driver.h"

#ifdef CONFIG_CONNECTION_SPI
static ledc_channel_config_t ledc_channel;
// LCD
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;
static const char *TAG = "oled-display-spi";

#elif CONFIG_CONNECTION_I2C
static const char *TAG = "oled-display-i2c";
#endif


/*
    Common for all displays
*/
void display_driver(void) {
    printf(" - Init: display_driver empty function call!\n\n");
    #ifdef CONFIG_CONNECTION_SPI
    ESP_LOGI(TAG, "DISP_SPI_SCLK: %d", DISP_SPI_SCLK);
    ESP_LOGI(TAG, "DISP_SPI_MOSI: %d", DISP_SPI_MOSI);
    ESP_LOGI(TAG, "DISP_SPI_RST: %d", DISP_SPI_RST);
    ESP_LOGI(TAG, "DISP_SPI_DC: %d", DISP_SPI_DC);
    ESP_LOGI(TAG, "DISP_SPI_CS: %d", DISP_SPI_CS);
    ESP_LOGI(TAG, "DISP_SPI_BL: %d", DISP_SPI_BL);
    #elif CONFIG_CONNECTION_I2C
    ESP_LOGI(TAG, "DISP_I2C_SDA: %d", DISP_I2C_SDA);
    ESP_LOGI(TAG, "DISP_I2C_SCL: %d", DISP_I2C_SCL);
    ESP_LOGI(TAG, "DISP_I2C_ADR: %d", DISP_I2C_ADR);
    #endif
}

/*
    LCD from waveshare board
*/
#ifdef CONFIG_CONNECTION_SPI  

esp_err_t display_init(void) {
    display_driver();
    // Skip SPI init, if SD Card is already there
    // LCD initialization - enable from example
    if (SDCard_Size) {
        ESP_LOGI(TAG, "Skip SPI Bus init at LCD as it was initialized at SD Card driver!");
    } else {
        ESP_LOGI(TAG, "Initialize SPI bus");
        spi_bus_config_t buscfg = { 
            .sclk_io_num = DISP_SPI_SCLK,
            .mosi_io_num = DISP_SPI_MOSI,
            .miso_io_num = GPIO_NUM_NC,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .max_transfer_sz = BUFFER_SIZE, // DISP_HOR_RES * DISP_DRAW_BUFF_HEIGHT * sizeof(uint16_t);
        };
        ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");
    }

    //  - repeat after example
    // https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/spi_lcd.html
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = DISP_SPI_DC,
        .cs_gpio_num = DISP_SPI_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx = &display,
    };
    
    // Attach the LCD to the SPI bus - repeat after example
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io_handle), TAG, "SPI init failed");

    /*
        RGB Order is usual (or can be absent):
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB
        However blue and red are shifted, so we need to set this (as at the example):
            .data_endian = LCD_RGB_ENDIAN_BGR,
        And later use directive LCD_CMD_INVON after display init:
            esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_INVON, NULL, 0);
        Now all colors are back to normal, no need to use other inversions.
    */
    
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = DISP_SPI_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .data_endian = LCD_RGB_ENDIAN_BGR,
        .flags = { .reset_active_high = 0 },  // Not in the example
    };
    
    ESP_LOGI(TAG, "Install ST7789T panel driver");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle), TAG, "Display init failed");

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    
    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); 

    /*
        Fix color inversion when black is white.
        Call last after init.
        #define LCD_CMD_INVON        0x21 // Go into display inversion mode
    */
    
    ESP_LOGI(TAG, "Set display inversion on, to fix black and white.");
    esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_INVON, NULL, 0);

    ESP_LOGI(TAG, "Display turned On");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on LCD backlight first, to see display content early!");
    BK_Init();  // Back light
    BK_Light(50);  // Less toxic
    return ESP_OK;
}

void BK_Init(void) {
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << DISP_SPI_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    
    // 配置LEDC
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel.channel    = LEDC_HS_CH0_CHANNEL;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = DISP_SPI_BL;
    ledc_channel.speed_mode = LEDC_LS_MODE;
    ledc_channel.timer_sel  = LEDC_HS_TIMER;
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);
}

void BK_Light(uint8_t Light) {
    ESP_LOGI(TAG, "Set LCD backlight");
    if(Light > 100) Light = 100;
    uint16_t Duty = LEDC_MAX_Duty-(81*(100-Light));
    if(Light == 0) Duty = 0;
    // 设置PWM占空比
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, Duty);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

}

/*
I2C Display
*/
#elif CONFIG_CONNECTION_I2C
esp_err_t display_i2c_init(void) {
    esp_err_t ret;
    display_driver();
    ESP_LOGI(TAG, "Run display i2c setup.");
    // I2C should be already init!
    i2c_master_bus_handle_t bus_handle;
    ret = master_bus_get(&bus_handle);
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C Bus should not be none at this stage!");
    }
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = DISP_I2C_ADR,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = LCD_CMD_BITS,           // According to SSD1306 datasheet
        .lcd_param_bits = LCD_CMD_BITS,         // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet

    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &io_config, &io_handle));

    return ESP_OK;
}
#endif