#include <cJSON.h>

void comm_Start();
void comm_Stop();

// void comm_DeleteAction(char* name, serverCommCallback callback);
void comm_AddActionStr(char* key, void(*callback)(char*));
void comm_AddActionInt32(char* key, void(*callback)(int32_t));
void comm_AddActionBool(char* key, void(*callback)(bool));
void comm_AddActionJson(char* key, void(*callback)(cJSON*));
void comm_AddActionVoid(char* key, void(*callback)(void));

void comm_PushMessage(char* key, char* value);

void comm_AddMessageStr(char* key, char*(*callback)());
void comm_AddMessageI32(char* key, int32_t(*callback)());
void comm_AddMessageFloat(char* key, float(*callback)());
void comm_AddMessageBool(char* key, bool(*callback)());
void comm_AddMessageJson(char* key, cJSON*(*callback)());
void comm_AddMessageVoid(char* key);