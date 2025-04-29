idf_component_register(
    SRCS 
                    "sensor_co2.c"
    PRIV_REQUIRES
                    i2c_driver
    INCLUDE_DIRS 
                    "include"
                    
)
