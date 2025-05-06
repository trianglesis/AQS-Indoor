
idf_build_get_property(target IDF_TARGET)

set(requires "display_driver" "lvgl" "esp_timer" "ui" )

idf_component_register(
        SRCS 
                        "lvgl_driver.c"
        REQUIRES        
                        "${requires}" 
        INCLUDE_DIRS    
                        "include"
)

message("REQUIRES:               'lvgl_driver':            ${requires}")