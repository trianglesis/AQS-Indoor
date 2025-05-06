idf_build_get_property(target IDF_TARGET)

set(requires "driver" "esp_timer")

# Display setup SPI
if(CONFIG_CONNECTION_SPI)
    list(APPEND requires esp_lcd lvgl_driver lvgl card_driver)
endif()

# Display setup I2C
if(CONFIG_CONNECTION_I2C)
    list(APPEND requires i2c_driver)
endif()

idf_component_register(
        SRCS 
                        "display_driver.c"
        REQUIRES 
                        "${requires}"
        INCLUDE_DIRS 
                        "include"
)
