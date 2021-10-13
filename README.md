# bike_navigator_esp_idf
## Overview
This project uses [Bike Navigator App](https://github.com/adam2809/BikeNavigatorApp/) to display cycling navigation on a display. So far I am targeting TTGO LoRa v2.0 board but I am planning to switch to a board that doesn't have LoRa but this will work with any ESP32 which has an SSD1306 128x64 display (either I2C or SPI).

## Configuration
To configure the display go to menuconfig option "SSD1306 Configuration" and set the correct display interface and pins.

## Installation
The installation is done via Platformio if you don't have it installed as an Atom or VS Code plugin install the [CLI version](https://platformio.org/install/cli) and run `platformio run --target upload`.
