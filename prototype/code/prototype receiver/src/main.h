#ifndef _MAIN_H
#define _MAIN_H

#include "setting.h"

#pragma region Pins Configuration
const int ENABLE_AP_PIN = 5;
const int PAIRING_PIN = 6;
const int SQUELCH_PIN = 2;
const int LEFT_DATA_PIN = 1;
const int RIGHT_DATA_PIN = 39;
const int RGB_LED_PIN = 38;
const int TEST_PIN = 15; //pull this high breifly when a packet is received to check timing jitter
// on the esp32-s3-devkitc-1 board i2c pins for the display
// SCL = 8
// SDA = 9
//-------------------------------------------------------------//
#pragma endregion

#endif