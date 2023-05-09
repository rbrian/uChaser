#ifndef _LED_H_
#define _LED_H_

#include <FastLED.h>

class LED
{
private:
#define NUM_LEDS 1
#define nLoops 5
#define ledLoopDelay 500

  CRGB leds[NUM_LEDS];
  CRGB loop[nLoops];
  int loopMax = -1;
  long lastmilli = 0;
  int curloop = 0;

protected:
  LED();

public:
  LED(LED &other) = delete;
  void operator=(const LED &) = delete;

  static LED *GetInstance();

  void Setup();
  void SetLED(CRGB color);
  void AddLoop(CRGB color);
  void RemoveLoop(CRGB color);
  void LoopLEDs();
};

#endif