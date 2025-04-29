
# TODO: Or move it into external component too?
# Add sources from SquareLine Studio export directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} 
                        "ui/exported/*.c"
                        "ui/exported/components/*.c"
                        "ui/exported/screens/*.c"
                        "ui/exported/images/*.c"
)

idf_component_register(
    SRCS 
                    "main.c"
                    ${SRC_UI}

    PRIV_REQUIRES 
                    spi_flash
    REQUIRES 
                    wifi
                    captive_portal
                    littlefs_driver
                    card_driver
                    webserver
                    display_driver
                    lvgl_driver
                    led_driver
                    ui
                    i2c_driver
                    sensor_co2
                    sensor_temp
    INCLUDE_DIRS 
                    ""
)

# External component, compile lvgl
# idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
# target_compile_options(${lvgl_lib} PRIVATE -Wno-format)

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
# littlefs_create_partition_image(littlefs ../flash_data FLASH_IN_PROJECT)