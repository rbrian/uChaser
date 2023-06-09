#include "led.h"
#include "main.h"
#include <FastLED.h>

static LED *led_ = nullptr;

LED::LED() {}

LED *LED::GetInstance()
{
  if (led_ == nullptr)
    led_ = new LED();
  return led_;
}

void LED::Setup()
{
  FastLED.addLeds<NEOPIXEL, RGB_LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(10);
}

void LED::SetLED(CRGB color)
{
  leds[0] = color;
  FastLED.show();
}

void LED::AddLoop(CRGB color)
{
  if (loopMax < nLoops - 1)
  {
    loopMax++;
    loop[loopMax] = color;
  }
}

void LED::RemoveLoop(CRGB color)
{
  for (int x = 0; x <= loopMax; x++)
  {
    if (loop[x] == color)
    {
      for (int y = x; y < loopMax; y++)
      {
        loop[y] = loop[y + 1];
      }
      loopMax--;
      break;
    }
  }
}

void LED::LoopLEDs()
{
  long m = millis();
  if (m > lastmilli + ledLoopDelay)
  {
    lastmilli = m;
    curloop++;
    if (curloop > loopMax)
    {
      curloop = 0;
    }
    SetLED(loop[curloop]);
  }
}
