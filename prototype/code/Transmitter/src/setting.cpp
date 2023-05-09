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
  Setting::GetInstance()->mac_address_string = (const char*)doc["mac_address"];
  Setting::GetInstance()->macStrToArr(mac_address_string, mac_address);
  Setting::GetInstance()->interval_milliseconds = doc["interval_milliseconds"];
  
  //output_method = doc["output_method"]; //todo - figure out why the datatypes don't work here, but do work in the photo booth project

  Serial.print("MAC Address from JSON: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(Setting::GetInstance()->mac_address[i], HEX);
    Serial.print(":");
  }
  Serial.println();
  Serial.print("Interval: ");
  Serial.println(Setting::GetInstance()->interval_milliseconds);

  return true;
  file.close();
}

bool Setting::writeJSON()
{ // write the global variables for speed etc to a file
  Serial.println("saving settings");
  StaticJsonDocument<512> doc;
  //doc["ssid"] = String(Setting::GetInstance()->ssid);
  //doc["password"] = String(Setting::GetInstance()->password);
  doc["interval_milliseconds"] = String(Setting::GetInstance()->interval_milliseconds);
  doc["mac_address"] = String(Setting::GetInstance()->mac_address_string);
  

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

void Setting::macStrToArr(const String& macString, uint8_t (&macArray)[6]) {
  // Create a static buffer to hold the resulting array
  //static uint8_t macArray[6];

  // Loop through each pair of characters in the MAC address string
  for (size_t i = 0; i < macString.length(); i += 2) {
    // Extract the two characters and convert them to a byte
    String byteString = macString.substring(i, i + 2);
    uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);

    // Store the byte in the output array
    macArray[i / 2] = byte;
  }
}


String Setting::macIntArrToStr(const uint8_t* macArray){
  String macString;

  // Loop through each byte in the array (assumes array size is 6)
  for (size_t i = 0; i < 6; i++) {
    // Convert the byte to a hexadecimal string
    String byteString = String(macArray[i], HEX);

    // If the byte is less than 0x10, add a leading zero
    if (macArray[i] < 0x10) {
      byteString = "0" + byteString;
    }

    // Concatenate the byte string to the MAC address string
    macString += byteString;
  }

  // Return the MAC address string
  return macString;
}