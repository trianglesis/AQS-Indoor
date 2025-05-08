
idf_build_get_property(target IDF_TARGET)

set(requires "display_driver" "sensor_co2" "sensor_temp" "lvgl" "esp_timer" "ui" )
set(priv_requires "battery_driver")

idf_component_register(
        SRCS 
                        "lvgl_driver.c"
        REQUIRES        
                        "${requires}" 
        PRIV_REQUIRES
                        "${priv_requires}"
        INCLUDE_DIRS    
                        "include"
)

# message("REQUIRES:               'lvgl_driver':            ${requires}")