Prototype receiver and transmitter:


4/13 Prototype Receiver Board notes:
SCL and SDA are swapped
Mark TX and RX as TX OUT and RX IN or something so it's clear
the diode drop from USB is too much, it makes the power module drop out.  Replace with a fet



Firmware wish list:
- Wifi captive portal
- Webserver with configuration page.  Can it do WIFI now at the same time as a webserver, possible to display live data?
- Settings saved to flash using SPIFFS/json
- Automatic pairing for ESP-NOW (via button press?) https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/
- RGB LED - what are all the statuses we should display on it?
    - Varying color based on angle?
    - Connected or not
    - Line of sight lost
- Automatic/assisted distance calibration?
- Enrich the ESP-NOW packet, what are the inputs/parameters?  Is this rapid enough to send analog signals for control?
- Logic for signal loss, averaging, add hysteresis (is this the right term?)
- Utilize multiple transmitting transceivers
- Measure the jitter in ESP-NOW interrupt timing on receiver
- Two way communication?
- UART/I2C/SPI/PWM outputs

Hardware wish list:
- Next revision of receiver board
- Power supply for transmitter suitable for battery operation
    - Voltage monitoring?
- Try out switching between multiple tx transcievers for better angle tolerance
- Figure out which pins to expose


State of this thing as of 4/5/23: transmitter sends on repeat, receiver calculates the distance and angle and puts it on the display

There are two main layers - RF and Ultrasonic.  The RF layer is wifi, using the ESP-NOW protocol.  The transmitting device sends a packet and the receiving one receives it (duh).

We are using the ESP32-S3 devkit board: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
and this adafruit OLED display: https://www.adafruit.com/product/4650

For now the circuits are identical, only the code is different.

![Schematic](ufollow-prototype-schematic.png)

RMT peripheral documentation (for generating waveforms/pulse sequences)
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html


For the revision 1 receiver:
- The squelch pin mutes the data output while it is high
- The data pin is normally low, transitions to high when a ping is received
- The squelch pin also resets the one-shot circuit, so:
1. Keep the squelch pin high normally
2. When a wifi packet is received, set that pin to low
3. When the data pin goes high (interrupt pin), log that timestamp and reset the squelch pin to high again
4. if too much time passes without the data pin changing, set the squelch high and wait for the next packet
5. Repeat forever


Component notes:
ESP32-S3 (this is the lowest model): https://www.lcsc.com/product-detail/WiFi-Modules_Espressif-Systems-ESP32-S3-WROOM-1-N4_C2913197.html

USB-UART CH340c: https://www.lcsc.com/product-detail/USB-ICs_WCH-Jiangsu-Qin-Heng-CH340C_C84681.html

DC-DC Module: https://www.lcsc.com/product-detail/Power-Modules_DEXU-Electronics-K7803M-1000R3_C2916516.html

almost 12 inch cable with JST XH connector: https://www.lcsc.com/product-detail/Dupont-Cable-Terminal-Block-Cable-Electronic-Cable_DEALON-LNN254-100724-300-4P_C5160861.html

Same cable but double ended: https://www.lcsc.com/product-detail/Dupont-Cable-Terminal-Block-Cable-Electronic-Cable_DEALON-LDS254-100724-300-4P_C5160865.html

XH vertical board connector:
- white: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_HCTL-XH-4A_C2908602.html
- black: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_HCTL-XH-4A-B_C2979570.html
- red: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_HCTL-XH-4A-R_C2979568.html
- blue: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_HCTL-XH-4A-L_C2979569.html

XH horizontal board connector: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_BOOMELE-Boom-Precision-Elec-XH-4AW_C21273.html

Qwiic
- Real JST: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_JST-Sales-America-BM04B-SRSS-TB-LF-SN_C160390.html
- Cheap clone: https://www.lcsc.com/product-detail/Wire-To-Board-Wire-To-Wire-Connector_JUSHUO-AFC10-S04QCA-00_C2764183.html