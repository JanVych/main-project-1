idf_component_register(SRCS "server_comm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES http_client
                    REQUIRES wifi
                    REQUIRES spi_flash
                    REQUIRES esp_app_format
                    REQUIRES nvs_flash)
