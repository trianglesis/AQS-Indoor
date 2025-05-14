


idf_component_register(
    SRCS 
                    "main.c"
    PRIV_REQUIRES 
                    spi_flash
    REQUIRES 
                    card_driver
                    littlefs_driver
                    battery_driver
                    captive_portal
                    display_driver
                    i2c_driver
                    led_driver
                    lvgl_driver
                    sensor_co2
                    sensor_temp
                    webserver
                    wifi
    INCLUDE_DIRS 
                    ""
)

# External component, compile lvgl
# idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
# target_compile_options(${lvgl_lib} PRIVATE -Wno-format)

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(littlefs ../flash_data FLASH_IN_PROJECT)