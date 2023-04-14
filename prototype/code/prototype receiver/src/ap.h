#ifndef _AP_H_
#define _AP_H_

#pragma region ui commands
#define WIFISSID 0
#define WIFIPASSWORD 1
#define WIFIRESTART 2
#define RESETDEFAULTS 3
#define RESETWIFI 4
#pragma endregion

class AccessPoint
{
private:
  AsyncWebServer server{80};
  AsyncWebSocket ws{"/ws"};

  DNSServer dnsServer;

  static void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  void internalOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  void publishSettings();
  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
  void inputProcessor(int command, String commandValue = "");

protected:
  AccessPoint();

public:
  AccessPoint(AccessPoint &other) = delete;
  void operator=(const AccessPoint &) = delete;

  static AccessPoint *GetInstance();

  void initWebSocket();
  void serverSetup();
  void startWifi();
  void stopWifi();

  bool Active = false;
};

#endif