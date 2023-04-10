#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "DNSServer.h"
#include "ap.h"
#include "setting.h"
#include "utils.h"

AccessPoint::AccessPoint()
{
  //  server(80);
  // ws("/ws");
  me = this;
}

void AccessPoint::initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void AccessPoint::serverSetup()
{
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/adv.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/adv.html", "text/html"); });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/bootstrap.min.js", "text/javascript"); });

  server.on("/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/jquery.min.js", "text/javascript"); });

  server.on("/js/scripts.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/js/scripts.js", "text/javascript"); });

  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/css/bootstrap.min.css", "text/css"); });

  server.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/css/style.css", "text/css"); });

  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/settings.json", "text/html"); });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
                      request->redirect("/index");
                      // Serial.print("server.notfound triggered: ");
                      // Serial.println(request->url());
                    });

  server.begin();
}

void AccessPoint::startWifi()
{
  WiFi.softAP(Setting::Current()->ssid.c_str(), Setting::Current()->password.c_str());
  Serial.println("Wifi Initialized");
  // This is the point that there will be a brownout when powered by USB

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  Active = true;
}

void AccessPoint::stopWifi()
{
  Serial.println("Disabling Wifi");
  WiFi.softAPdisconnect(true);

  Active = false;
}

// Websocket stuff - send and receive
void AccessPoint::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  me->internalOnEvent(server, client, type, arg, data, len);
}

void AccessPoint::internalOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    this->publishSettings(); // send initial state to any client on connecting
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    this->handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

// TODO: change text letter codes to #defined stuff
void AccessPoint::publishSettings()
{ // push the acceleration, interval, and steps per degree to the web interface, on boot only?  no changes possible from the physical buttons
  Serial.println("Publishing settings to websocket");
  ws.textAll(String("U" + Setting::Current()->ssid));
  ws.textAll("V" + Setting::Current()->password);
}

void AccessPoint::handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{ // take stuff from the websocket, shoot it to the input processor
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    Serial.println((char *)data);
    String inString = (char *)data;

    if (strcmp((char *)data, "restartwifi") == 0)
    { // restart wifi
      Setting::Current()->writeJSON();
      this->inputProcessor(WIFIRESTART);
    }

    else if (strcmp((char *)data, "resetdefaults") == 0)
    {
      this->inputProcessor(RESETDEFAULTS);
    }

    else if (strcmp((char *)data, "resetwifi") == 0)
    {
      this->inputProcessor(RESETWIFI);
    }

    else if (inString.substring(0, 1) == String("u"))
    { // ssid
      inString.remove(0, 1);
      this->inputProcessor(WIFISSID, inString); // should be a string
    }

    else if (inString.substring(0, 1) == String("v"))
    { // password
      inString.remove(0, 1);
      inputProcessor(WIFIPASSWORD, inString); // should be a string.  will this fail if it is blank, may need to handle it?
    }
  }
}

/* Purpose of this function is to take all the various inputs form physical buttons or websocket,
 * decide what to do based on the current state of the motor (is it moving or not?),
 * apply the needed changes (change speed, mode, start motion or stop it)
 * then finally update the things that changed back to the websocket
 *
 * the command value is ignored for move, direction, and enable
 */
void AccessPoint::inputProcessor(int command, String commandValue)
{
  bool SaveSettings = true; // set to false in a case to avoid writing json
  switch (command)
  {
  case WIFISSID:
    Serial.println("Update SSID Command");
    Setting::Current()->ssid = commandValue;
    break;
  case WIFIPASSWORD:
    Serial.println("Update Password Command");
    Setting::Current()->password = commandValue;
    break;
  case WIFIRESTART:
    Serial.println("Restart Wifi Command");
    Utils::reboot();
    break;
  case RESETDEFAULTS:
    Serial.println("Reset default settings Command");
    Setting::Current()->reset();
    publishSettings();
    break;
  case RESETWIFI:
    Serial.println("Reset wifi credentials Command");
    Setting::Current()->resetWifi();
    Setting::Current()->writeJSON(); // have to do this here or it will reboot before saving
    Utils::reboot();
    break;
  }
  if (SaveSettings)
  { // save settings to JSON, except speed command because it can come so rapidly
    Setting::Current()->writeJSON();
  }
}
