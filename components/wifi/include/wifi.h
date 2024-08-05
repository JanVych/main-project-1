void wifiSTAConnect(char *ssid, char *password);
void wifiSTADisconnect();
void wifiDestroy();

bool wifiIsRunning();
bool wifiIsSTAConnected();
char* wifiGetSTASsid();
char* wifiGetAvailableNetworks();