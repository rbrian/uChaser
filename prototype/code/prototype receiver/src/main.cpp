
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
#include <AverageValue.h>


//----------------------Pins Configuration---------------------//
const int BUTTON_C_PIN = 6;
const int SQUELCH_PIN = 2;
const int LEFT_DATA_PIN = 18;
const int RIGHT_DATA_PIN = 39;

//on the esp32-s3-devkitc-1 board i2c pins for the display
//SCL = 8
//SDA = 9
//-------------------------------------------------------------//


unsigned long lastPacketMicros = 0;
volatile bool waitingForPacket = true;
volatile long lastLeftMicros = 0;
volatile long lastRightMicros = 0;
volatile bool leftIntWaiting = false;
volatile bool rightIntWaiting = false;

int angle = 0;
int distance = 0;

const int pingTimeoutMicros = 100000; //1000 microseconds in a millisecond
const long distOffsetMicros = 10; //offset to calibrate the distance calculation.  This represents the time between receiving the packet and receiving the ping, excluding the ping travel time.
const int avgSampleNum = 6; //number of samples for the running averages
const int sensorSpacingMm = 223; //millimeters between each ultrasonic receiver
const float speedOfSoundMmUs = .343; //millimeters per microsecond
const float speedOfSoundMUs = 3.43;

AverageValue<long> averageAngle(avgSampleNum);
AverageValue<long> averageDistance(avgSampleNum);

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


//Program states
#define WAITINGFORPACKET 0
#define WAITINGFORPINGS 1
#define DOINGMATH 2
#define STILLWAITINGFORPINGS 3
#define GOTBOTHPINGS 4
#define TIMEDOUT 5
int progState = WAITINGFORPACKET;


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
  display.print("Angle: ");
  display.println(angle);
  display.print("Distance: ");
  display.println(distance);
  display.display(); 
}

void IRAM_ATTR packetReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&packet, incomingData, sizeof(packet));
  
  if(waitingForPacket){ //if waiting, it means we should do stuff.  Otherwise it means this was triggered but something else is already being processed
    waitingForPacket = false;
  }

  currentSequenceNo = packet.sequenceNo;
}

void IRAM_ATTR leftPingReceived(){
  if (leftIntWaiting){
    lastLeftMicros = micros();
    leftIntWaiting = false;
  }
}

void IRAM_ATTR rightPingReceived(){
  if (rightIntWaiting){
    lastRightMicros = micros();
    rightIntWaiting = false;
  }
}

int waitForPings() {
  if (leftIntWaiting == false && rightIntWaiting == false){
    //if we're at this point, both pings should be logged
    Serial.println("Got both pings");
    return GOTBOTHPINGS;
  }
  else if (micros() > lastPacketMicros + pingTimeoutMicros){
    //if this happens, we have timed out
    Serial.println("Timed out");
    return TIMEDOUT;
  }
  else {
    //if we don't have both pings and haven't timed out, it means we're still waiting
    return STILLWAITINGFORPINGS;
  }
}

void doMath() {
  averageAngle.push(lastLeftMicros - lastRightMicros);
  averageDistance.push((lastLeftMicros + lastRightMicros) / 2 - lastPacketMicros - distOffsetMicros); //this is storing the calibrated travel time

  angle = acos((speedOfSoundMmUs*averageAngle.average())/sensorSpacingMm) * 57296 / 1000; //end up with an int for the angle.  What are these magic numbers?

  distance = constrain(averageDistance.average() * speedOfSoundMmUs, 10, 100000); //int should be millimeters
}

void reset() {
  //reset all the flags for a new loop
  leftIntWaiting = false;
  rightIntWaiting = false;
  waitingForPacket = true;
  //disable the receivers until the next packet shows up
  digitalWrite(SQUELCH_PIN, HIGH);
}

bool waitForPacket(){

  if(waitingForPacket){
    //if waiting for packet is still true, packet interrupt is just hanging out, and we return false so that the program doesn't advance to the next state
    return false;
  }
  else {
    //if the packet interrupt has changed it's waiting flag to true, it means we should now be waiting for echoes and the program should advance to the next state
    //record the time of the packet
    lastPacketMicros = micros();

    //enable the receivers
    digitalWrite(SQUELCH_PIN, LOW);

    //enable the echo interrupts
    leftIntWaiting = true;
    rightIntWaiting = true;
    
    return true;
  }
}

void display_setup() {
  delay(1000); //todo: probably don't need this delay
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

  pinMode(SQUELCH_PIN, OUTPUT);
  digitalWrite(SQUELCH_PIN, HIGH);

  pinMode(LEFT_DATA_PIN, INPUT);
  attachInterrupt(LEFT_DATA_PIN, leftPingReceived, RISING);

  pinMode(RIGHT_DATA_PIN, INPUT);
  attachInterrupt(RIGHT_DATA_PIN, rightPingReceived, RISING);

  
  Serial.println("Setup Complete");
  

}

void loop() {
  if (progState == WAITINGFORPACKET){
    int packetStatus;
    packetStatus = waitForPacket();
      if (packetStatus == true) {
        //If this comes back true, it means we have received a packet, logged the time, and triggered the sensors
        //so we can move on to the next state.  If not, it just keeps running the waitForPacket function.
        progState = WAITINGFORPINGS;
      }
    }

  else if (progState == WAITINGFORPINGS){
      //Serial.println("Waiting for pings"); // for some reason without this line, we get repeats of the previous message in domath(), looks timing related
      int pingStatus;
      pingStatus = waitForPings();
      if (pingStatus == TIMEDOUT) {
        //If it timed out, we should reset everything and go back to waiting for a packet
        reset();
        progState = WAITINGFORPACKET;
      }
      else if (pingStatus == GOTBOTHPINGS){
        //If we have both pings, let's move on to doing math
        progState = DOINGMATH;

      }
  }

  else if (progState == DOINGMATH){
      //Serial.println("Doing math");
      //once we get to this state, just do the math once, then reset for a new loop
      doMath();
      updateDisplay();
      reset();
      progState = WAITINGFORPACKET;
  }

  else {
    Serial.print("Uh oh, how did we get here?");
  }

  
}