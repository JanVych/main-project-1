idf_component_register(SRCS "http_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES http_client
                    REQUIRES esp_http_client
                    REQUIRES esp_netif)
