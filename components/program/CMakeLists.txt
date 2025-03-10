idf_component_register(SRCS "program_test.c" "program.c"
                    INCLUDE_DIRS "include"
                    REQUIRES server_comm
                    REQUIRES wattrouter_mx)
