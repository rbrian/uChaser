
// To do:
// Figure out how to get serial.print to output from the native USB port on the devkit board, it wants to come out of the USB-UART chip (hardware serial pins)
// Replace all the serial print commands with something that can be toggled - for debug only

// There is some kind of bug - when booting the receiver after it is flashed, if it receives a packet (maybe before completing setup?) it has a kernel panic
// Perhaps this is because the interrupt is getting triggered before some other parts are initialized

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <OneButton.h>
#include <AverageValue.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <fastled.h>
#include "main.h"
#include "led.h"
#include "ap.h"
#include "SPIFFS.h"

unsigned long lastPacketMicros = 0;
volatile bool waitingForPacket = true;
volatile long lastLeftMicros = 0;
volatile long lastRightMicros = 0;
volatile bool leftIntWaiting = false;
volatile bool rightIntWaiting = false;

int angle = 0;
int distance = 0;

const int pingTimeoutMicros = 100000; // 1000 microseconds in a millisecond
const long distOffsetMicros = 10;     // offset to calibrate the distance calculation.  This represents the time between receiving the packet and receiving the ping, excluding the ping travel time.
const int avgSampleNum = 6;           // number of samples for the running averages
const int sensorSpacingMm = 223;      // millimeters between each ultrasonic receiver
const float speedOfSoundMmUs = .343;  // millimeters per microsecond
const float speedOfSoundMUs = 3.43;

AverageValue<long> averageAngle(avgSampleNum);
AverageValue<long> averageDistance(avgSampleNum);

//------------------WIFI-NOW Configuration---------------------//
// MAC address that the packet should be sent to, get this from the receiving device
// Example receiver device is 58:BF:25:9E:D7:B0
// uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x9E, 0xD7, 0xB0};

// The ESP-NOW protocol allows up to 250 bytes to be sent at a time (or received in this case)
typedef struct incoming_packet
{
  int32_t sequenceNo = 0;
} incoming_packet;

typedef struct pairing_packet {       // new structure for pairing
    uint8_t msgType;
    uint8_t macAddr[6];
    uint8_t channel;
} pairing_packet;

incoming_packet packet;
pairing_packet pairing;
uint8_t pairingAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
esp_now_peer_info_t peerInfo;

int32_t currentSequenceNo = 0;
//-------------------------------------------------------------//

//-------------------Display Configuration---------------------//
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
//-------------------------------------------------------------//

//--------------------Button Configuration---------------------//
OneButton pairingButton(PAIRING_PIN, true);
OneButton wifiButton(ENABLE_AP_PIN, true);
//-------------------------------------------------------------//

// Initialize some things

// Program states
#define WAITINGFORPACKET 0
#define WAITINGFORPINGS 1
#define DOINGMATH 2
#define STILLWAITINGFORPINGS 3
#define GOTBOTHPINGS 4
#define TIMEDOUT 5
#define WIFIAP 6
#define PAIRING 7
int progState = WAITINGFORPACKET; //default mode when the main loop starts

void updateDisplay()
{
  // for testing just put something on the display
  display.clearDisplay();
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("MAC Address:");
  display.println(WiFi.macAddress());
  display.print("Sequence No: ");
  display.println(currentSequenceNo);
  display.print("Millis: ");
  display.println(millis());
  display.print("Angle: ");
  display.println(angle);
  display.print("Distance: ");
  display.println(distance);
  display.display();
}

void IRAM_ATTR packetReceived(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  //this assumes all packets are a ping, if there are other kinds we should differentiate between them
  memcpy(&packet, incomingData, sizeof(packet));

  if (waitingForPacket)
  { // if waiting, it means we should do stuff.  Otherwise it means this was triggered but something else is already being processed
    waitingForPacket = false;
  }

  currentSequenceNo = packet.sequenceNo;
}

void IRAM_ATTR leftPingReceived()
{
  if (leftIntWaiting)
  {
    lastLeftMicros = micros();
    leftIntWaiting = false;
  }
}

void IRAM_ATTR rightPingReceived()
{
  if (rightIntWaiting)
  {
    lastRightMicros = micros();
    rightIntWaiting = false;
  }
}

int waitForPings()
{
  if (leftIntWaiting == false && rightIntWaiting == false)
  {
    // if we're at this point, both pings should be logged
    Serial.println("Got both pings");
    return GOTBOTHPINGS;
  }
  else if (micros() > lastPacketMicros + pingTimeoutMicros)
  {
    // if this happens, we have timed out
    Serial.println("Timed out");
    return TIMEDOUT;
  }
  else
  {
    // if we don't have both pings and haven't timed out, it means we're still waiting
    return STILLWAITINGFORPINGS;
  }
}

void doMath()
{
  averageAngle.push(lastLeftMicros - lastRightMicros);
  averageDistance.push((lastLeftMicros + lastRightMicros) / 2 - lastPacketMicros - distOffsetMicros); // this is storing the calibrated travel time

  angle = acos((speedOfSoundMmUs * averageAngle.average()) / sensorSpacingMm) * 57296 / 1000; // end up with an int for the angle.  What are these magic numbers?

  distance = constrain(averageDistance.average() * speedOfSoundMmUs, 10, 100000); // int should be millimeters
}

void reset()
{
  // reset all the flags for a new loop
  leftIntWaiting = false;
  rightIntWaiting = false;
  waitingForPacket = true;
  // disable the receivers until the next packet shows up
  digitalWrite(SQUELCH_PIN, HIGH);
}

bool waitForPacket()
{

  if (waitingForPacket)
  {
    // if waiting for packet is still true, packet interrupt is just hanging out, and we return false so that the program doesn't advance to the next state
    return false;
  }
  else
  {
    // if the packet interrupt has changed it's waiting flag to true, it means we should now be waiting for echoes and the program should advance to the next state
    // record the time of the packet
    lastPacketMicros = micros();

    // enable the receivers
    digitalWrite(SQUELCH_PIN, LOW);

    // enable the echo interrupts
    leftIntWaiting = true;
    rightIntWaiting = true;

    return true;
  }
}

void display_setup()
{
  delay(1000); // todo: probably don't need this delay
  display.begin(0x3C, true);
  display.clearDisplay();
  Serial.println("OLED begun");
  updateDisplay();
}

void esp_now_setup()
{
  // Turn on wifi in station mode
  WiFi.mode(WIFI_STA);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Channel: ");
  Serial.println(WiFi.channel());

  pairing.channel = WiFi.channel();
  WiFi.macAddress(pairing.macAddr);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, pairingAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(packetReceived);
}

void wifiButtonPress()
{
  Serial.println("Wifi button pushed");
  if (progState == WIFIAP){
    //Stop the AP and webserver
    //todo: if the interrupts were disabled, turn them back on
    LED::GetInstance()->SetLED(CRGB::Red);
    progState = WAITINGFORPACKET;
    AccessPoint::GetInstance()->stop();
  }
  else {
    //Start the AP and webserver
    //todo: should the interrupts be disabled?
    progState = WIFIAP;
    AccessPoint::GetInstance()->start();
    LED::GetInstance()->SetLED(CRGB::Blue);
  }  
}

void pairingButtonPress(){
  if (progState == WIFIAP) {
    return;
  }
  progState = PAIRING;
  //it may not be necessary to change the program state if everything is contained within this function.  Actually it would be better to use a callback to return the program state
  Serial.print("Sending out pairing packet to transmitter");
  //switch program modes for a moment
  //send out a packet to all addresses
  //when the transmitter gets it, it should update it's target mac address to this receiver's mac address
  //receiver can just go back to listening mode

    //the values sent out in this thing are set in esp_now_setup()+
    esp_err_t result = esp_now_send(pairingAddress, (uint8_t *) &pairing, sizeof(pairing));

    if (result == ESP_OK) {
      Serial.println("... Success");
    }
    else {
      Serial.print("... Failure: ");
      Serial.println(esp_err_to_name(result));
    }

  reset();
  progState = WAITINGFORPACKET;
}

void setup()
{
  Serial.begin(115200);
  esp_now_setup();
  display_setup();

  LED::GetInstance()->Setup();
  LED::GetInstance()->SetLED(CRGB::Red);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Setting::GetInstance()->readJSON();

  wifiButton.attachClick(wifiButtonPress);
  pairingButton.attachClick(pairingButtonPress);

  pinMode(SQUELCH_PIN, OUTPUT);
  digitalWrite(SQUELCH_PIN, HIGH);

  pinMode(LEFT_DATA_PIN, INPUT);
  attachInterrupt(LEFT_DATA_PIN, leftPingReceived, RISING);

  pinMode(RIGHT_DATA_PIN, INPUT);
  attachInterrupt(RIGHT_DATA_PIN, rightPingReceived, RISING);

  

  Serial.println("Setup Complete");

  
}

void loop()
{
  wifiButton.tick();
  pairingButton.tick();

  if (progState == WAITINGFORPACKET)
  {
    int packetStatus;
    packetStatus = waitForPacket();
    if (packetStatus == true)
    {
      // If this comes back true, it means we have received a packet, logged the time, and triggered the sensors
      // so we can move on to the next state.  If not, it just keeps running the waitForPacket function.
      progState = WAITINGFORPINGS;
    }
  }

  else if (progState == WAITINGFORPINGS)
  {
    // Serial.println("Waiting for pings"); // for some reason without this line, we get repeats of the previous message in domath(), looks timing related
    int pingStatus;
    pingStatus = waitForPings();
    if (pingStatus == TIMEDOUT)
    {
      // If it timed out, we should reset everything and go back to waiting for a packet
      updateDisplay();
      reset();
      progState = WAITINGFORPACKET;
    }
    else if (pingStatus == GOTBOTHPINGS)
    {
      // If we have both pings, let's move on to doing math
      progState = DOINGMATH;
    }
  }

  else if (progState == DOINGMATH)
  {
    // Serial.println("Doing math");
    // once we get to this state, just do the math once, then reset for a new loop
    doMath();
    updateDisplay();
    reset();
    progState = WAITINGFORPACKET;
  }

  else if(progState == WIFIAP)
  {
    //Do nothing in the loop for this state
    //the webserver and AP should have been started by the function that put it into this state
    //and we're relying on the webserver stuff to get us out of this state - by putting us back into WAITINGFORPACKET or perhaps just rebooting
    //todo: should there be a continuous output in this mode, to indicate there is no direction/distance, or maybe the robot connected should halt?
    //Serial.println("WIFI AP mode");
  }
  else if(progState == PAIRING)
  {
    //Do nothing in the loop for this state
    //a packet will be sent out to the transmitter, then the function that sent it will return the program to a regular mode
  }

  else
  {
    Serial.print("Uh oh, how did we get here?");
  }
}