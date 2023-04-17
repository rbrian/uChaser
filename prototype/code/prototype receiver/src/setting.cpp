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

void Setting::restoreDefaults()
{ // reset to default values
  return;
}

bool Setting::readJSON()
{ // read the JSON file and load values into global variables
  Serial.println("loading settings");
  File file = SPIFFS.open("/settings.json", "r");
  if (!file)
  {
    Serial.println(F("Failed to read file"));
    return false;
  }

  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError err = deserializeJson(doc, file);
  if (err)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(err.c_str());
    file.close();
    return false;
  }

  //read the values
  num_avg_samples = doc["num_avg_samples"];
  distance_between_receivers_mm = doc["distance_between_receivers_mm"];
  //output_method = doc["output_method"]; //todo - figure out why the datatypes don't work here, but do work in the photo booth project

  Serial.print("Samples: ");
  Serial.println(num_avg_samples);
  Serial.print("Distance between receivers: ");
  Serial.println(distance_between_receivers_mm);
  Serial.print("Output method: ");
  Serial.println(output_method);

  return true;
  file.close();
}

bool Setting::writeJSON()
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
    return false;
  }

  if (serializeJson(doc, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
    configFile.close();
    return false;
  }

  configFile.close();
  return true;
}

