idf_component_register(
            SRCS 
                        "battery_driver.c"
            REQUIRES
                        esp_adc driver esp_timer
            INCLUDE_DIRS 
                        "include"
)
