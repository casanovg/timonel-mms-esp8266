/*
  Timonel bootloader I2C-master multi slave application demo for ESP8266
  ............................................................................
  File: timonel-mms-esp8266.h (Header)
  ............................................................................
  This demo shows how to control and update several Tiny85 microcontrollers
  running the Timonel bootloader from an ESP8266 master.
  It uses a serial console configured at 115200 N 8 1 for feedback.
  ............................................................................
  Version: 1.5.0 / 2020-07-13 / gustavo.casanova@nicebots.com
  ............................................................................
*/

#ifndef TIMONEL_MMS_ESP8266_H
#define TIMONEL_MMS_ESP8266_H

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

// This software
#define VER_MAJOR 1
#define VER_MINOR 5
#define VER_PATCH 0

// Serial display settings
#define USE_SERIAL Serial
#define SERIAL_BPS 115200

// I2C pins
#define SDA 2  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 0  // I2C SCL pin - ESP8266 0 - ESP32 22

// Routine settings
#define MAX_TWI_DEVS 28
#define LOOP_COUNT 3
#define T_SIGNATURE 84

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
Timonel::Status PrintStatus(Timonel *timonel);
void ThreeStarDelay(void);
void ShowHeader(void);
void PrintLogo(void);
void ClrScr(void);

#endif  // TIMONEL_MMS_ESP8266_H
