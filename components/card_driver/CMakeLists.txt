idf_component_register(
    
    SRCS 
                "card_driver.c"
    PRIV_REQUIRES 
                vfs sdmmc spi_flash
    REQUIRES 
                fatfs mbedtls
    INCLUDE_DIRS 
                "include"
)
