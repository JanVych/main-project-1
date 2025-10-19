idf_component_register(SRCS "program_wattrouter_ecomax.c" "program_dp_led_test.c" "program_eta.c" "program.c"
                    INCLUDE_DIRS "include"
                    REQUIRES server_comm
                    REQUIRES wattrouter_mx
                    REQUIRES driver
                    REQUIRES ecomax
                    REQUIRES etatherm
                    REQUIRES freertos)
