idf_component_register(
    SRCS 
                    "sensor_co2.c"
                    "sensiniron/sensirion_common.c"
                    "sensiniron/sensirion_i2c.c"
                    "sensiniron/scd4x_i2c.c"
    PRIV_REQUIRES
                    i2c_driver esp_timer led_driver
    INCLUDE_DIRS 
                    "include"
                    "./sensiniron/"
)
