#include "setting.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>

static Setting *setting_ = nullptr;

Setting::Setting(){}

Setting *Setting::GetInstance()
{
  if (setting_ == nullptr)
    setting_ = new Setting();
  return setting_;
}

void Setting::reset()
{ // reset most things, but not wifi
}

void Setting::resetWifi()
{
  ssid = d_ssid;
  password = d_password;
}

void Setting::writeJSON()
{ // write the global variables for speed etc to a file
  Serial.println("saving settings");
  StaticJsonDocument<512> doc;
  doc["ssid"] = String(Setting::GetInstance()->ssid);
  doc["password"] = String(Setting::GetInstance()->password);

  SPIFFS.remove("/settings.json");

  File configFile = SPIFFS.open("/settings.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open settings file for writing");
  }

  if (serializeJson(doc, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }

  configFile.close();
}

