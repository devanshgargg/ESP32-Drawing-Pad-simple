# ESP OLED Draw Pad

A real-time web-based drawing pad built using an ESP8266 / ESP32 and a
0.96" SSD1306 OLED display.

Draw on a phone, tablet, or computer through a web browser and see your
drawing appear live on the OLED display over Wi-Fi.

Draw on your phone and see it live on the 0.96" OLED display. 
<p align="center"> 
   <img src="images/project image.jpeg" width="360" alt="ESP8266 OLED Web Notepad">
</p>

##  How It Works

The ESP microcontroller creates a web server and connects to the local
Wi-Fi network.

1. Connect the ESP8266 / ESP32 to Wi-Fi.
2. Open the device's web page from a phone or computer.
3. Draw inside the web drawing area.
4. The drawing data is sent to the ESP through Wi-Fi.
5. The ESP processes the drawing coordinates.
6. The SSD1306 OLED displays the drawing in real time.

## Hardware

- ESP8266 or ESP32
- 0.96" SSD1306 OLED (128x64, I2C)
- Jumper wires
- Breadboard

## Wiring

| OLED I2C | ESP8266 | ESP32 |
|------|----------|-------|
| VCC  | 3.3V     |  3.3V
| GND  | GND      | GND |
| SDA  | D2 (GPIO4) | D21 (GPIO21) |
| SCL  | D1 (GPIO5) | D22 (GPIO22) |

## Libraries

- ESP8266WiFi or ESP32WiFi
- ESPAsyncWebServer
- Adafruit_GFX
- Adafruit_SSD1306

## Upload

1. Install the required libraries.
2. Select **NodeMCU** or **ESP32**
3. Upload `OLED_DrawPad.ino`.
4. Open the Serial Monitor to get the IP address (192.168.4.1).
5. Open that IP address in your phone browser.

<p align="center">
   <img src="images/web image.png" width="338" alt="ESP8266 OLED Web Notepad"> 
</p> 
## Features

-  Real-time drawing through a web browser
-  Works from phone, tablet, or computer
-  Live drawing output on a 0.96" SSD1306 OLED
-  Wi-Fi based communication
-  Fast I²C communication at 400 kHz
-  Supports both ESP8266 and ESP32
-  Same firmware supports both boards
-  No dedicated mobile application required
-  Simple and lightweight web interface.

## Future Improvements

- Undo / Redo
- Save drawings
- Touch pressure simulation
- WebSocket optimization

## Author

**DEVANSH GARG**
