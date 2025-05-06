idf_build_get_property(target IDF_TARGET)

# Display setup SPI
if(CONFIG_CONNECTION_SPI)
    set(requires "display_driver")
endif()


idf_component_register(
        SRCS 
                        "lvgl_driver.c"
        REQUIRES 
                        "${requires}"
                        "lvgl" 
                        "esp_timer" 
        INCLUDE_DIRS 
                        "include"
)

message("REQUIRES:               'lvgl_driver':         ${requires}")