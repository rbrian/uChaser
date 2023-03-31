Prototype receiver and transmitter:

There are two main layers - RF and Ultrasonic.  The RF layer is wifi, using the ESP-NOW protocol.  The transmitting device sends a packet and the receiving one receives it (duh).

We are using the ESP32-S3 devkit board: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html
and this adafruit OLED display: https://www.adafruit.com/product/4650

For now the circuits are identical, only the code is different.

![Schematic](ufollow-prototype-schematic.png)