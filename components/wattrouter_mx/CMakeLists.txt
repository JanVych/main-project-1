idf_component_register(SRCS "rs485_modbus_rtu.c" "wattrouter_mx.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer
                    REQUIRES driver)
