#include <cJSON.h>
typedef struct http_response_t
{
    char *data;
    uint16_t data_len;  
    char *content_type;
    cJSON *json;
} http_response_t;

void httpGet(char *url, http_response_t *response);
void httpPost(char *url, char* data, http_response_t *response);
void httpGetJson(char *url, http_response_t *response);
void httpPostJson(char *url, cJSON* json, http_response_t *response);