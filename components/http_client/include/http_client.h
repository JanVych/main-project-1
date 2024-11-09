#include <cJSON.h>
#include "esp_check.h"
#include <stdbool.h>

typedef struct http_response_t
{
    uint32_t status;
    char *data;
    uint32_t data_len;  
    char *content_type;
    cJSON *json;
} http_response_t;


void http_Get(char* url, http_response_t* response);
void http_Post(char* url, char* data, http_response_t* response);
void http_GetJson(char* url, http_response_t* response);
void http_PostJson(char* url, cJSON* json, http_response_t* response);

http_response_t* http_CreateResponse();
void http_DeleteResponse(http_response_t* response);
void http_CleanResponse(http_response_t* r);

esp_err_t http_BuildUrl(bool tls_ssl, const char *host, uint16_t port, const char *path, const char *query, char *url, int max_len); 