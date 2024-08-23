idf_component_register(SRCS "server_comm.c"
                    INCLUDE_DIRS "include"
                    REQUIRES http_client
                    REQUIRES wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_app_format
                    REQUIRES esp_https_ota
                    REQUIRES esp_partition)
