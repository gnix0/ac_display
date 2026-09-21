idf_component_register(SRCS "display.c" "amoled_driver.c" "initSequence.c"
                        PRIV_REQUIRES lvgl esp_driver_gpio esp_driver_spi spi_flash
                        INCLUDE_DIRS "include" ".")