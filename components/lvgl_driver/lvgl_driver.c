#include "lvgl_driver.h"

static const char *TAG = "lvgl";

lv_disp_t *display;

void lvgl_driver(void) {
    printf(" - Init: lvgl_driver empty function call!\n\n");
    ESP_LOGI(TAG, "CONNECTION_SPI: %d", CONNECTION_SPI);
    ESP_LOGI(TAG, "CONNECTION_I2C: %d", CONNECTION_I2C);
    ESP_LOGI(TAG, "BUFFER_SIZE: %d", BUFFER_SIZE);
    ESP_LOGI(TAG, "RENDER_MODE: %d", RENDER_MODE);
    ESP_LOGI(TAG, "Display rotation is set to %d degree! Offsets X: %d Y: %d", ROTATE_DEGREE, Offset_X, Offset_Y);
}

static void lvgl_tick_increment(void *arg) {
    // Tell LVGL how many milliseconds have elapsed
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_tick_init(void) {
    // Tick interface for LVGL (using esp_timer to generate 5ms periodic event)
    ESP_LOGI(TAG, "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_increment,
        .name = "LVGL_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));
    // Next create a task
}

static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    // this gets called when the DMA transfer of the buffer data has completed
    lv_display_t *disp_driver = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp_driver);
    return false;
}

/* 
    Rotate display, when rotated screen in LVGL. Called when driver parameters are updated. 
    There is no HAL?
    Must change Offset Y to X at flush_cb
    Offset_Y 34  // 34 IF ROTATED 270deg
*/
static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    
    /* If Rotated And if this is SPI OLED from Waheshare
        https://forum.lvgl.io/t/gestures-are-slow-perceiving-only-detecting-one-of-5-10-tries/18515/86 
        If not - offset will be 0 which is ok
    */
    int x1 = area->x1 + Offset_X;
    int x2 = area->x2 + Offset_X;
    int y1 = area->y1 + Offset_Y;
    int y2 = area->y2 + Offset_Y;
    
    // Different way forr monoch disp
    #ifdef CONFIG_CONNECTION_I2C
    // To use LV_COLOR_FORMAT_I1, we need an extra buffer to hold the converted data
    static uint8_t oled_buffer[BUFFER_SIZE];

    uint16_t hor_res = lv_display_get_physical_horizontal_resolution(disp);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            /* The order of bits is MSB first
                        MSB           LSB
               bits      7 6 5 4 3 2 1 0
               pixels    0 1 2 3 4 5 6 7
                        Left         Right
            */
            bool chroma_color = (px_map[(hor_res >> 3) * y  + (x >> 3)] & 1 << (7 - x % 8));

            /* Write to the buffer as required for the display.
            * It writes only 1-bit for monochrome displays mapped vertically.*/
            uint8_t *buf = oled_buffer + hor_res * (y >> 3) + (x);
            if (chroma_color) {
                (*buf) &= ~(1 << (y % 8));
            } else {
                (*buf) |= (1 << (y % 8));
            }
        }
    }
    #endif // CONFIG_CONNECTION
    
    // Choose for both methods, easier to read
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    #ifdef CONFIG_CONNECTION_SPI
    // SPI coloured
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    #elif CONFIG_CONNECTION_I2C
    // I2C mono
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, oled_buffer);
    #endif // CONFIG_CONNECTION
}

static void set_resolution(lv_display_t* disp) {
    // Enforce if needed
    lv_display_set_resolution(disp, DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_physical_resolution(disp, DISP_HOR_RES, DISP_VER_RES);
}

/*
Simple for I2C display
*/
static void lvgl_task(void * pvParameters)  {
    ESP_LOGI(TAG, "Starting LVGL task");
    // Wait TS between cycles real time
    TickType_t last_wake_time  = xTaskGetTickCount();
    long curtime = esp_timer_get_time()/1000;
    int counter = 0;
    
    lv_obj_t *scr = lv_display_get_screen_active(display);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_obj_set_width(label, lv_display_get_horizontal_resolution(display));
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(label, "Hello Espressif, Hello LVGL.");

    // Show text 3 sec
    vTaskDelay(pdMS_TO_TICKS(3000));
    // Handle LVGL tasks
    while (1) {
        _lock_acquire(&lvgl_api_lock);  // Lock the mutex due to the LVGL APIs are not thread-safe
        lv_task_handler();
        if (esp_timer_get_time()/1000 - curtime > 1000) {
            curtime = esp_timer_get_time()/1000;
            char textlabel[20];
            sprintf(textlabel, "Running: %u\n", counter);
            printf(textlabel);
            lv_label_set_text_fmt(label, "Runninc cycles: %d - run!", counter);
            counter++;
        } // Wait for next update
        _lock_release(&lvgl_api_lock);  // Actual sleep real time?
        xTaskDelayUntil(&last_wake_time, DISPLAY_UPDATE_FREQ);
    } // WHILE
}

void graphics_i2c_draw(void) {
    // Create a set of tasks to read sensors and update LCD, LED and other elements
    // xTaskCreatePinnedToCore(lvgl_task, "i2c display task", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(lvgl_task, "i2c display task", 8192, NULL, 9, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "Display LVGL Scroll Text");
}

/*
Task to process complex UI with OLED display from Waveshare board via SPI
*/
void graphics_spi_draw(void) {
    // Use as task!
    ui_init_fake(); // NOTE: Always init UI from SquareLine Studio export!

}

esp_err_t lvgl_init(void) {
    ESP_LOGI(TAG, "Initialize LVGL library");
    // Init
    lv_init(); 

    display = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_user_data(display, panel_handle);
    lv_display_set_default(display); // Set this display as default for UI use

    #ifdef CONFIG_CONNECTION_SPI
    // Buffers
    void* buf1 = NULL;
    void* buf2 = NULL;
    buf1 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);
    buf2 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);
    assert(buf1);
    assert(buf2);
    #elif CONFIG_CONNECTION_I2C
    void* buf1 = NULL;
    size_t draw_buffer_sz = BUFFER_SIZE + LVGL_PALETTE_SIZE;
    buf1 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_8BIT);
    assert(buf1);
    #endif // CONFIG_CONNECTION

    // Different buffer usage
    #ifdef CONFIG_CONNECTION_SPI
    // Two buffers for coured OLED, render PARTIAL
    lv_display_set_buffers(display, buf1, buf2, BUFFER_SIZE, RENDER_MODE);
    #elif CONFIG_CONNECTION_I2C
    lv_display_set_color_format(display, LV_COLOR_FORMAT_I1);
    // One buffer for mono OLED, render FULL
    lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz, RENDER_MODE);
    #endif // CONFIG_CONNECTION

    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, flush_cb);
    
    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_flush_ready,
    };
    // W (1442) lcd_panel.io.spi: Callback on_color_trans_done was already set and now it was overwritten!

    /* Register done callback */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display), TAG, "esp_lcd_panel_io_register_event_callbacks error"); // I have tried to use 
    
    lvgl_tick_init(); // timer

    /* Landscape orientation:
        270deg = USB on the left side - landscape orientation
        270deg = USB on the right side - landscape orientation 
    90 
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        esp_lcd_panel_mirror(panel_handle, true, false);
        esp_lcd_panel_swap_xy(panel_handle, true);

    180
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
        esp_lcd_panel_mirror(panel_handle, true, true);
        esp_lcd_panel_swap_xy(panel_handle, false);
    270
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
        esp_lcd_panel_mirror(panel_handle, false, true);
        esp_lcd_panel_swap_xy(panel_handle, true);
    See: https://forum.lvgl.io/t/gestures-are-slow-perceiving-only-detecting-one-of-5-10-tries/18515/60
    */
    if (ROTATE_DEGREE == 0) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    } else if (ROTATE_DEGREE == 90) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        esp_lcd_panel_mirror(panel_handle, true, false);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else if (ROTATE_DEGREE == 180) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
        esp_lcd_panel_mirror(panel_handle, true, true);
        esp_lcd_panel_swap_xy(panel_handle, false);
    } else if (ROTATE_DEGREE == 270) {
        ESP_LOGI(TAG, "Rotating display by: %d deg", ROTATE_DEGREE);
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
        esp_lcd_panel_mirror(panel_handle, false, true);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    }


    // Sometimes better to hard-set resolution
    #ifdef CONFIG_CONNECTION_SPI
    set_resolution(display);
    #endif // CONFIG_CONNECTION

    #ifdef CONFIG_CONNECTION_SPI
    // Drop any theme if exist
    bool is_def = lv_theme_default_is_inited();
    if (is_def) {
        // drop the default theme
        ESP_LOGI(TAG, "Drop the default theme");
        lv_theme_default_deinit();
    }
    #endif // CONFIG_CONNECTION

    return ESP_OK;
}