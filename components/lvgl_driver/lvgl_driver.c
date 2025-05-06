#include "lvgl_driver.h"

static const char *TAG = "lvgl";

// LVGL drawing
static void* buf1 = NULL;
static void* buf2 = NULL;

lv_disp_t *display;


void lvgl_driver(void) {
    printf(" - Init: lvgl_driver empty function call!\n\n");
    ESP_LOGI(TAG, "CONNECTION_SPI: %d", CONNECTION_SPI);
    ESP_LOGI(TAG, "CONNECTION_I2C: %d", CONNECTION_I2C);
    ESP_LOGI(TAG, "Display rotation is set to %d degree! Offsets X: %d Y: %d", ROTATE_DEGREE, Offset_X, Offset_Y);
}

// Tell LVGL how many milliseconds have elapsed
void lvgl_tick_increment(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static esp_err_t lvgl_tick_init(void) {
    esp_timer_handle_t  tick_timer;
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_increment,
        .name = "LVGL tick",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&lvgl_tick_timer_args, &tick_timer), TAG, "Creating LVGL timer filed!");
    return esp_timer_start_periodic(tick_timer, 2 * 1000); // 2 ms
}

// this gets called when the DMA transfer of the buffer data has completed
bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
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
void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    /* 
        If Rotated
        https://forum.lvgl.io/t/gestures-are-slow-perceiving-only-detecting-one-of-5-10-tries/18515/86
    */
    int x1 = area->x1 + Offset_X;
    int x2 = area->x2 + Offset_X;
    int y1 = area->y1 + Offset_Y;
    int y2 = area->y2 + Offset_Y;

    /*
        There is usually rgb565 enforcing, but it no longer needed.
        See how it was implemented in main\display_driver\ST7789V3.c at the panel_config code
        lv_draw_sw_rgb565_swap(px_map, (x2 + 1 - x1) * (y2 + 1 - y1));
    */
    
    esp_lcd_panel_draw_bitmap((esp_lcd_panel_handle_t)lv_display_get_user_data(disp), x1, y1, x2 + 1, y2 + 1, px_map);
}

void set_resolution(lv_display_t* disp) {
    // 
    lv_display_set_resolution(disp, DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_physical_resolution(disp, DISP_HOR_RES, DISP_VER_RES);
}

/*
Simple for I2C display
*/
void example_lvgl_demo_ui(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, "Hello Espressif, Hello LVGL.");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    long curtime = esp_timer_get_time()/1000;
    int counter = 0;

    // Handle LVGL tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();

        if (esp_timer_get_time()/1000 - curtime > 1000) {
            curtime = esp_timer_get_time()/1000;

            char textlabel[20];
            sprintf(textlabel, "Running: %u\n", counter);
            printf(textlabel);
            lv_label_set_text(label, textlabel);
            counter++;
        }
    }
}

esp_err_t graphics_i2c_draw(void) {

    example_lvgl_demo_ui(display);

    return ESP_OK;
}

/*
Task to process complex UI with OLED display from Waveshare board via SPI
*/
esp_err_t graphics_spi_draw(void) {
    
    // Use as task!
    long curtime = esp_timer_get_time()/1000;
    ui_init_fake(); // NOTE: Always init UI from SquareLine Studio export!
    // Handle LVGL tasks
    // REPEAT THE 'static void lvgl_task(void * pvParameters)' from older example

    return ESP_OK;

}

esp_err_t lvgl_init(void) {
    ESP_LOGI(TAG, "Initialize LVGL library");
    // Init
    lv_init(); 

    /*
        Changed the default setup from example with LVGL 8 version to a newer implementation.
        Old lv_disp_drv_init(); and lv_disp_drv_register();
    */

    display = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    
    // Buffers
    buf1 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);
    buf2 = heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_INTERNAL |  MALLOC_CAP_DMA);

    /*
        Changed the default setup from example with LVGL 8 version to a newer implementation.
        Old: lv_disp_draw_buf_init();
    */
    lv_display_set_buffers(display, buf1, buf2, BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_display_set_user_data(display, panel_handle);

    /*
        You can force the display to operate in proper color mode, but it's no longer required:
        See how it was implemented in main\display_driver\ST7789V3.c at the panel_config code
        lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
        // Another approach to fix color inversion, also no longer required, 
        keep it here just as a point for other people.
        esp_lcd_panel_invert_color(panel_handle, true);
    */

    lv_display_set_flush_cb(display, flush_cb);
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_flush_ready,
    };
    // W (1442) lcd_panel.io.spi: Callback on_color_trans_done was already set and now it was overwritten!

    /* Register done callback */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display), TAG, "esp_lcd_panel_io_register_event_callbacks error"); // I have tried to use 
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "esp_lcd_panel_init error");

    // Sometimes better to hard-set resolution
    set_resolution(display);

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
    
    // Manual rotate 270deg, no HAL sensor
    if (ROTATE_DEGREE == 0) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    } else if (ROTATE_DEGREE == 90) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        esp_lcd_panel_mirror(panel_handle, true, false);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else if (ROTATE_DEGREE == 180) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_180);
        esp_lcd_panel_mirror(panel_handle, true, true);
        esp_lcd_panel_swap_xy(panel_handle, false);
    } else if (ROTATE_DEGREE == 270) {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_270);
        esp_lcd_panel_mirror(panel_handle, false, true);
        esp_lcd_panel_swap_xy(panel_handle, true);
    } else {
        lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);
    }

    // Set this display as defaulkt for UI use
    lv_display_set_default(display);

    #ifdef CONFIG_CONNECTION_SPI
    // Drop any theme if exist
    bool is_def = lv_theme_default_is_inited();
    if (is_def) {
        // drop the default theme
        ESP_LOGI(TAG, "Drop the default theme");
        lv_theme_default_deinit();
    }
    #endif // CONFIG_CONNECTION

    // Init LVGL, then use appropriate display and graphics for it
    esp_err_t ret = lvgl_tick_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Timer failed to initialize");
    }
    return ESP_OK;
}