idf_component_register(
            SRCS 
                        "battery_driver.c"
            REQUIRES
                        esp_adc driver esp_timer sqlite_driver
            INCLUDE_DIRS 
                        "include"
)
