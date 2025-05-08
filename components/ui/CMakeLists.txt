idf_build_get_property(target IDF_TARGET)

set(requires "lvgl")
set(include_dirs "include")

# Add sources from SquareLine Studio export directory
if(CONFIG_CONNECTION_SPI)
        file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} 
                                "sq_line_ST7789V3/*.c"
                                "sq_line_ST7789V3/components/*.c"
                                "sq_line_ST7789V3/screens/*.c"
                                "sq_line_ST7789V3/images/*.c"
        )
        list(APPEND requires "sq_line_ST7789V3")
        list(APPEND include_dirs "sq_line_ST7789V3")
elseif (CONFIG_CONNECTION_I2C)
        file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} 
                                "sq_line_SSD1306/*.c"
                                "sq_line_SSD1306/components/*.c"
                                "sq_line_SSD1306/screens/*.c"
                                "sq_line_SSD1306/images/*.c"
        )
        list(APPEND requires "sq_line_SSD1306")
        list(APPEND include_dirs "sq_line_SSD1306")
endif()

idf_component_register(
        SRCS 
                        "ui_load.c"
                        ${SRC_UI}
        REQUIRES        
                        "${requires}" 
        INCLUDE_DIRS    
                        "${include_dirs}"
)

# message("SRC_UI:             'ui_load':            ${SRC_UI}")
# message("requires:           'ui_load':            ${requires}")
# message("include_dirs:       'ui_load':            ${include_dirs}")