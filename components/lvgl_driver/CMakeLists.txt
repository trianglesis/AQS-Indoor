
idf_component_register(
        SRCS 
                        "lvgl_driver.c"
        REQUIRES 
                        "display_driver"
                        "lvgl" 
                        "esp_timer" 
                        "ui" 
        INCLUDE_DIRS 
                        "include"
)

message("REQUIRES:               'lvgl_driver':         ${REQUIRES}")