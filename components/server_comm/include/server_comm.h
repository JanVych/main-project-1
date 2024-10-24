void commStart();
void commStop();

typedef void (*serverCommCallback)(char*);
void commAddAction(char* name, serverCommCallback callback);
void commDeleteAction(char* name, serverCommCallback callback);

void commAddMessage(char* key, char* value);