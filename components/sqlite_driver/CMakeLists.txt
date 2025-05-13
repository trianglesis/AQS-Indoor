idf_component_register(
    
                SRCS 
                                "sqlite_driver.c"
                PRIV_REQUIRES
                                card_driver
                REQUIRES
                                sqlite3
                INCLUDE_DIRS 
                                "include"
)
