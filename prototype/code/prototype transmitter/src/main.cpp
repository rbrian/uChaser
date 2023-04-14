
//To do:
//Figure out how to get serial.print to output from the native USB port on the devkit board, it wants to come out of the USB-UART chip (hardware serial pins)
//Replace all the serial print commands with something that can be toggled - for debug only

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <driver/rmt.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <OneButton.h>
#include <SimpleTimer.h>


//----------------------Pins Configuration---------------------//
gpio_num_t PULSE_PIN = GPIO_NUM_18;
const int BUTTON_C_PIN = 6;

//on the esp32-s3-devkitc-1 board i2c pins
//SCL = 8
//SDA = 9
//-------------------------------------------------------------//


//------------------WIFI-NOW Configuration---------------------//
//MAC address that the packet should be sent to, get this from the receiving device
//Example receiver device is 58:BF:25:9E:D7:B0
uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0xE3, 0x90, 0xAC};
//7C:DF:A1:E3:90:AC
//The ESP-NOW protocol allows up to 250 bytes to be sent at a time
typedef struct esp_packet {
  int32_t sequenceNo = 0;
} esp_packet;

esp_packet packet;
esp_now_peer_info_t peerInfo;

int32_t currentSequenceNo = 0;
//-------------------------------------------------------------//


//-----------------------RMT Configuration---------------------//
//Use the RMT peripheral to generate pulses https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html
//The output waveform is a list of: on duration, off duration, on level, off level
//these parameters are used to construct that list.  Durations are in microseconds when the clock divider is 80

const int pulseCount = 8;
const int highDuration = 10;
const int lowDuration = 10;

rmt_config_t rconfig;
rmt_item32_t pulses[pulseCount];
//-------------------------------------------------------------//


//-------------------Display Configuration---------------------//
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
//-------------------------------------------------------------//


//--------------------Button Configuration---------------------//
OneButton testButton(BUTTON_C_PIN, true);
//-------------------------------------------------------------//

//--------------------Simpletimer------------------------------//
const int packetInterval = 500; //milliseconds

SimpleTimer timer;
//-------------------------------------------------------------//

void test_pulse(){
  rmt_write_items(rconfig.channel, pulses, pulseCount, 0);
}



void rmt_setup() {
  rconfig.rmt_mode = RMT_MODE_TX;
  rconfig.channel = RMT_CHANNEL_0;
  rconfig.gpio_num = PULSE_PIN;
  rconfig.mem_block_num = 1;
  rconfig.tx_config.loop_en = 0;
  rconfig.tx_config.carrier_en = 0;
  rconfig.tx_config.idle_output_en = 1;
  rconfig.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  rconfig.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  rconfig.clk_div = 80; // 80MHx / 80 = 1MHz 0r 1uS per count
 
  rmt_config(&rconfig);
  rmt_driver_install(rconfig.channel, 0, 0);

  for (int i = 0; i < pulseCount; i++){
    pulses[i].duration0 = lowDuration;
    pulses[i].level0 = 0;
    pulses[i].duration1 = highDuration;
    pulses[i].level1 = 1;
  }

}

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
  display.print(millis());
  display.display(); 
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

  //Assign a callback function to be triggered transmission is complete
  //This callback function might be the best place to trigger the ultrasonic ping
  //esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void send_packet() {
  //Update the packet object with new values if necessary
  currentSequenceNo++;

  packet.sequenceNo = currentSequenceNo;

  //Send the current values of the packet object to the receiver
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));

  //output a pulse for the transducer
  test_pulse();


  //print something to the serial port
  if (result == ESP_OK) {
    Serial.print("Sent packet number: ");
  Serial.println(currentSequenceNo);
  }
  else {
    Serial.println("Error sending the data");
  }
  
  //and finally update the display
  updateDisplay();
}

void button_setup() {
  testButton.attachClick(send_packet);
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
  rmt_setup();
  display_setup();
  button_setup();

  timer.setInterval(packetInterval, send_packet); //use this to send packets all the time instead of on button presses only
  Serial.println("Setup Complete");
  

}

void loop() {
  testButton.tick();
  //scanI2cAddresses();
  timer.run();


  
}