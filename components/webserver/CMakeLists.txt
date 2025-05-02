idf_component_register(
    SRCS 
            "webserver.c"
    PRIV_REQUIRES 
            vfs sdmmc spi_flash
    REQUIRES
            littlefs_driver card_driver
    INCLUDE_DIRS 
            "include"
)
