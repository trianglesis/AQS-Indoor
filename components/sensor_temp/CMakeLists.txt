idf_component_register(
    SRCS 
                    "sensor_temp.c"
    REQUIRES
                    esp_bme680 esp_type_utils
    PRIV_REQUIRES
                    i2c_driver esp_timer sqlite_driver
    INCLUDE_DIRS 
                    "include"
                    
)
