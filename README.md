# timonel-mms-esp8266
## Timonel bootloader I2C-master multi slave application demo for ESP8266

This ESP8266/Arduino test application [shows](https://youtu.be/PM9X1thrdOY) the usage of TWI master libraries.

It is a serial console-based application that runs a 3-time loop that flashes, deletes and runs a user application on several Tiny85's (bare chips or Digispark) running the [Timonel bootloader](https://github.com/casanovg/timonel).

The application has been tested on ESP-01 and NodeMCU modules. It is compiled and flashed to the device using [PlatformIO](http://platformio.org) over [VS Code](http://code.visualstudio.com). The [arduino folder](/arduino/Timonel-MMS-ESP8266) contains the same application, adapted to be used through the Arduino IDE.
