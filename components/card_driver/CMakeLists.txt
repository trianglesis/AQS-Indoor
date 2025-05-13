idf_component_register(
    
    SRCS 
                "card_driver.c"
    REQUIRES 
                vfs sdmmc spi_flash fatfs mbedtls sqlite_driver
    INCLUDE_DIRS 
                "include"
)
