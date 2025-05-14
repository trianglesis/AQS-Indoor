idf_component_register(
    
                SRCS 
                                "sqlite_driver.c"
                REQUIRES
                                esp_timer card_driver sqlite3
                INCLUDE_DIRS 
                                "include"
)
