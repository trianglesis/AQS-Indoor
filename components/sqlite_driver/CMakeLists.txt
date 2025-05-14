idf_component_register(
    
                SRCS 
                                "sqlite_driver.c"

                REQUIRES
                                card_driver battery_driver sensor_co2 sensor_temp esp_timer sqlite3
                INCLUDE_DIRS 
                                "include"
)
