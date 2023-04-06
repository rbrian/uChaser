Prototype receiver and transmitter:

State of this thing as of 4/5/23: transmitter sends on repeat, receiver calculates the distance and angle and puts it on the display

There are two main layers - RF and Ultrasonic.  The RF layer is wifi, using the ESP-NOW protocol.  The transmitting device sends a packet and the receiving one receives it (duh).

We are using the ESP32-S3 devkit board: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
and this adafruit OLED display: https://www.adafruit.com/product/4650

For now the circuits are identical, only the code is different.

![Schematic](ufollow-prototype-schematic.png)

RMT peripheral documentation (for generating waveforms/pulse sequences)
https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/rmt.html


For the revision 1 receiver:
-The squelch pin mutes the data output while it is high
-The data pin is normally low, transitions to high when a ping is received
-The squelch pin also resets the one-shot circuit, so:
1. Keep the squelch pin high normally
2. When a wifi packet is received, set that pin to low
3. When the data pin goes high (interrupt pin), log that timestamp and reset the squelch pin to high again
4. if too much time passes without the data pin changing, set the squelch high and wait for the next packet
5. Repeat forever
