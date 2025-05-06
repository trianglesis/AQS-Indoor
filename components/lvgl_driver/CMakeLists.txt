# Display setup SPI
if(CONFIG_CONNECTION_SPI)
    set(requires display_driver)
endif()

idf_component_register(
    
        SRCS 
                        "lvgl_driver.c"
        REQUIRES 
                        "${requires}"
                        lvgl esp_timer display_driver
        INCLUDE_DIRS 
                        "include"

)
