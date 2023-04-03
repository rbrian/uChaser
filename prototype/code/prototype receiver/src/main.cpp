
//To do:
//Figure out how to get serial.print to output from the native USB port on the devkit board, it wants to come out of the USB-UART chip (hardware serial pins)
//Replace all the serial print commands with something that can be toggled - for debug only

//There is some kind of bug - when booting the receiver after it is flashed, if it receives a packet (maybe before completing setup?) it has a kernel panic
//Perhaps this is because the interrupt is getting triggered before some other parts are initialized



#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <OneButton.h>


//----------------------Pins Configuration---------------------//
const int BUTTON_C_PIN = 6;
const int SQUELCH_PIN = 2;
const int DATA_PIN = 18;

//on the esp32-s3-devkitc-1 board i2c pins
//SCL = 8
//SDA = 9
//-------------------------------------------------------------//

const int squelchTimeoutMs = 75;


bool displayNeedsUpdating = true;
int startUs = 0;
int deltaUs = 0; //number of microseconds between the packet and the last ping from the receiver


//------------------WIFI-NOW Configuration---------------------//
//MAC address that the packet should be sent to, get this from the receiving device
//Example receiver device is 58:BF:25:9E:D7:B0
//uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x9E, 0xD7, 0xB0};

//The ESP-NOW protocol allows up to 250 bytes to be sent at a time (or received in this case)
typedef struct esp_packet {
  int32_t sequenceNo = 0;
} esp_packet;

esp_packet packet;
//esp_now_peer_info_t peerInfo;

int32_t currentSequenceNo = 0;
//-------------------------------------------------------------//


//-------------------Display Configuration---------------------//
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
//-------------------------------------------------------------//


//--------------------Button Configuration---------------------//
OneButton testButton(BUTTON_C_PIN, true);
//-------------------------------------------------------------//



void updateDisplay(){
  //for testing just put something on the display
  display.clearDisplay();
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.println("MAC Address:");
  display.println(WiFi.macAddress());
  display.print("Sequence No: ");
  display.println(currentSequenceNo);
  display.print("Millis: ");
  display.println(millis());
  display.print("Delta MS: ");
  display.println(deltaUs);
  display.display(); 
  Serial.println(currentSequenceNo); //todo: this is temporary, just to make sure the thing is working while troubleshooting the display
  displayNeedsUpdating = false;
}

void IRAM_ATTR packetReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&packet, incomingData, sizeof(packet));
  
  currentSequenceNo = packet.sequenceNo;
  startUs = micros();
  digitalWrite(SQUELCH_PIN, LOW);
  //updateDisplay(); //it may be better to set a flag, then watch for it in the loop to update the display... not sure if calling this function from here will make the interrupt callback take too long
  displayNeedsUpdating = true;
}

void IRAM_ATTR pingReceived(){
  //measure the time, then squelch
  deltaUs = micros() - startUs;
  digitalWrite(SQUELCH_PIN, HIGH);
  displayNeedsUpdating = true;
  Serial.println("ping");
}

void display_setup() {
  delay(1000); //todo: probably don't need this
  display.begin(0x3C, true);
  display.clearDisplay();
  Serial.println("OLED begun");
  updateDisplay();
}

void esp_now_setup() {
  //Turn on wifi in station mode
  WiFi.mode(WIFI_STA);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  //Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(packetReceived);

}



void button_setup() {
  //testButton.attachClick(test_pulse);
}

void scanI2cAddresses(){
    byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}



void setup() {
  Serial.begin(115200);
  esp_now_setup();
  display_setup();
  //button_setup();
  pinMode(SQUELCH_PIN, OUTPUT);
  digitalWrite(SQUELCH_PIN, HIGH);

  pinMode(DATA_PIN, INPUT);
  attachInterrupt(DATA_PIN, pingReceived, RISING);

  
  Serial.println("Setup Complete");
  

}

void loop() {
  //testButton.tick();
  //scanI2cAddresses();
  // updateDisplay();
  // delay(500);

  if(displayNeedsUpdating){
    updateDisplay();
  }

  
}