#include <esp_now.h>
#include "utils.h"
#include "Esp.h"

void Utils::reboot()
{
  delay(1000);
  ESP.restart();
}
