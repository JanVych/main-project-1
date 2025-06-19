idf_component_register(SRCS "ecomax.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver
                    REQUIRES esp_timer
                    REQUIRES helper)
