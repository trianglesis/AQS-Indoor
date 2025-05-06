idf_component_register(
    
    SRCS 
                "card_driver.c"
    PRIV_REQUIRES 
                vfs sdmmc
    REQUIRES 
                spi_flash fatfs mbedtls
    INCLUDE_DIRS 
                "include"
)
