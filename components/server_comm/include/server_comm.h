#include <cJSON.h>

void comm_Start();
void comm_Stop();

typedef void (*serverCommCallback)(char*);
void comm_AddAction(char* name, serverCommCallback callback);
void comm_DeleteAction(char* name, serverCommCallback callback);

void comm_PushMessage(char* key, char* value);

void comm_AddMessageStr(char* key, char*(*callback)());
void comm_AddMessageI32(char* key, int32_t(*callback)());
void comm_AddMessageBool(char* key, bool(*callback)());
void comm_AddMessageCjson(char* key, cJSON*(*callback)());