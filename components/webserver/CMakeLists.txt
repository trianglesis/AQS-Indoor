idf_component_register(
    SRCS 
            "webserver.c"
    PRIV_REQUIRES 
            vfs sdmmc spi_flash
    REQUIRES
            wifi littlefs_driver card_driver esp-tls esp_http_server
    INCLUDE_DIRS 
            "include"
)
