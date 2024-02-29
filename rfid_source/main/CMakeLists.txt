idf_component_register(SRCS "main.c" "esp-idf-rc522/src/rc522.c"
                    INCLUDE_DIRS "."
                    REQUIRES bt
                    REQUIRES driver
                    REQUIRES nvs_flash)
