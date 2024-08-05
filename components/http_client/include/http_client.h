#include <cJSON.h>
typedef struct http_response_t
{
    uint32_t status;
    char *data;
    uint32_t data_len;  
    char *content_type;
    cJSON *json;
} http_response_t;

void httpGet(char *url, http_response_t *response);
void httpPost(char *url, char* data, http_response_t *response);
void httpGetJson(char *url, http_response_t *response);
void httpPostJson(char *url, cJSON* json, http_response_t *response);

http_response_t* httpCreateResponse();
void httpDeleteResponse(http_response_t *response);
void httpCleanResponse(http_response_t *r);