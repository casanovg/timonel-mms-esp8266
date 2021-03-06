/*
  Timonel bootloader I2C-master multi slave application demo for ESP8266
  ............................................................................
  File: timonel-mms-esp8266.ino (Arduino sketch)
  ............................................................................
  This demo shows how to control and update several Tiny85 microcontrollers
  running the Timonel bootloader from an ESP8266 master.
  It uses a serial console configured at 115200 N 8 1 for feedback.
  ............................................................................
  Version: 1.5.0 / 2020-07-13 / gustavo.casanova@nicebots.com
  ............................................................................
*/

/*
 Working routine:
 ----------------
   1) Scans the TWI bus in search of all devices running Timonel.
   2) Creates an array of Timonel objects, one per device.
   3) Deletes existing firmware of each device.
   4) Uploads "avr-blink-twis.hex" application payload to each device.
   5) Launches application on each device, let it run 10 seconds.
   6) Sends reset command to the application: led blinking should stop on all devices.
   7) Repeats the routine 3 times.
*/

#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>

#include "payload.h"

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

/* ___________________
  |                   | 
  |    Setup block    |
  |___________________|
*/
void setup() {
    // Initialize the serial port for debugging
    USE_SERIAL.begin(SERIAL_BPS);
    ClrScr();
    PrintLogo();
    ShowHeader();
    /* ____________________
      |                    | 
      |    Routine loop    |
      |____________________|
    */
    for (uint8_t loop = 0; loop < LOOP_COUNT; loop++) {
        USE_SERIAL.printf_P("\n\rPASS %d OF %d ...\n\r", loop + 1, LOOP_COUNT);
        // The bus device scanning it has to be made as fast as possible since each
        // discovered Timonel has to be initialized before launching the user apps
        TwiBus twi_bus(SDA, SCL);
        TwiBus::DeviceInfo dev_info_arr[HIG_TWI_ADDR - LOW_TWI_ADDR + 1];
        // Scanning the TWI bus in search of devices ...
        uint8_t tml_count = 0;
        USE_SERIAL.printf_P("\n\r");
        while (tml_count == 0) {
            USE_SERIAL.printf_P("\r\x1b[5mScanning TWI bus ...\x1b[0m");
            twi_bus.ScanBus(dev_info_arr, HIG_TWI_ADDR - LOW_TWI_ADDR + 1, LOW_TWI_ADDR);
            uint8_t arr_size = (sizeof(dev_info_arr) / sizeof(dev_info_arr[0]));
            for (uint8_t i = 0; i < arr_size; i++) {
                if (dev_info_arr[i].firmware == "Timonel") {
                    tml_count++;
                }
            }
            if (tml_count > 0) {
                USE_SERIAL.printf_P("\rTimonel devices found: %d\n\r", tml_count);
            } else {
                if (dev_info_arr[0].addr) {
                    USE_SERIAL.printf_P("\rDevice found at address %d NOT responding, resetting both micros ...\n\r", dev_info_arr[0].addr);
                    NbMicro *micro = new NbMicro(dev_info_arr[0].addr, SDA, SCL);
                    micro->TwiCmdXmit(NO_OP, UNKNOWNC);
                    micro->TwiCmdXmit(RESETMCU, ACKRESET);
                    delete micro;
                    delay(5000);
                    ESP.restart();
                }
            }
            delay(1000);
        }
        Timonel *tml_pool[tml_count];
        //
        // **************************************************
        // * Create and initialize bootloader objects found *
        // **************************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                tml_pool[i] = new Timonel(dev_info_arr[i].addr, SDA, SCL);
                USE_SERIAL.printf_P("\n\rGetting status of Timonel device %d\n\r", dev_info_arr[i].addr);
                Timonel::Status sts = tml_pool[i]->GetStatus();
                if ((sts.features_code >> F_APP_AUTORUN) & true) {
                    USE_SERIAL.printf_P("\n\r ***************************************************************************************\n\r");
                    USE_SERIAL.printf_P(" * WARNING! The Timonel bootloader with TWI address %02d has the \"APP_AUTORUN\" feature. *\n\r", dev_info_arr[i].addr);
                    USE_SERIAL.printf_P(" * enabled. This TWI master firmware can't control it properly! Please recompile it    *\n\r");
                    USE_SERIAL.printf_P(" * using a configuration with that option disabled (e.g. \"tml-t85-small\").             *\n\r");
                    USE_SERIAL.printf_P(" ***************************************************************************************\n\r");
                }
            }
        }
        USE_SERIAL.printf_P("\n\r");
        ThreeStarDelay();
        USE_SERIAL.printf_P("\n\n\r");
        //
        // *********************************************
        // * Delete user applications from all devices *
        // *********************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                delay(10);
                USE_SERIAL.printf_P("Deleting application on device %d ", dev_info_arr[i].addr);
                uint8_t errors = tml_pool[i]->DeleteApplication();
                // delay(500);
                // tml_pool[i]->TwiCmdXmit(RESETMCU, ACKRESET);
                // delay(500);
                if (errors == 0) {
                    USE_SERIAL.printf_P("OK!\n\r");
                } else {
                    USE_SERIAL.printf_P("Error! (%d)\n\r", errors);
                    //Wire.begin(SDA, SCL);
                }
                delay(1000);
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
                tml_pool[i]->GetStatus();
                PrintStatus(tml_pool[i]);
            }
        }
        ThreeStarDelay();
        USE_SERIAL.printf_P("\n\r");
        //
        // ***************************************************
        // * Upload and run user applications on all devices *
        // ***************************************************
        for (uint8_t i = 0; i <= (tml_count); i++) {
            if (dev_info_arr[i].firmware == "Timonel") {
                USE_SERIAL.printf_P("\n\rUploading application to device %d, \x1b[5mPLEASE WAIT\x1b[0m ...", dev_info_arr[i].addr);
                uint8_t errors = tml_pool[i]->UploadApplication(payload, sizeof(payload));
                if (errors == 0) {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      \n\r");
                } else {
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)           \n\r", errors);
                }
                delay(10);
                USE_SERIAL.printf_P("\n\rGetting status of device %d\n\r", dev_info_arr[i].addr);
                tml_pool[i]->GetStatus();
                PrintStatus(tml_pool[i]);
                delay(10);
                // // If the Timonel features support it, dump the device memory
                // Timonel::Status sts = tml_pool[i]->GetStatus();
                // if ((sts.features_code >> F_CMD_READFLASH) & true) {
                //     USE_SERIAL.printf_P("\n\rDumping device %d flash memory\n\r", dev_info_arr[i].addr);
                //     tml_pool[i]->DumpMemory(MCU_TOTAL_MEM, SLV_PACKET_SIZE, 32);
                // }
                USE_SERIAL.printf_P("Running application on device %d\n\r", dev_info_arr[i].addr);
                errors = tml_pool[i]->RunApplication();
                delay(500);
                if (errors == 0) {
                    USE_SERIAL.printf_P("User application should be running\n\r");
                } else {
                    USE_SERIAL.printf_P("Bootloader exit to app error! (%d)           \n\r", errors);
                }
                delay(10);
                delete tml_pool[i];
                delay(1500);
            }
        }
        //
        // ************************************************************************
        // * Reset applications and prepare for another cycle, then clean objects *
        // ************************************************************************
        if (loop < LOOP_COUNT - 1) {
            uint8_t dly = 10;
            USE_SERIAL.printf_P("\n\rLetting application run %d seconds before resetting and starting next cycle   ", dly);
            while (dly--) {
                USE_SERIAL.printf_P("\b\b| ");
                delay(250);
                USE_SERIAL.printf_P("\b\b/ ");
                delay(250);
                USE_SERIAL.printf_P("\b\b- ");
                delay(250);
                USE_SERIAL.printf_P("\b\b\\ ");
                delay(250);
            }
            USE_SERIAL.printf_P("\b\b* ");
            USE_SERIAL.printf_P("\n\n\r");
            // Resetting devices
            // NOTE: All devices share the same application, since the application TWI address is
            // set at compile time, this is shared across all devices when the app is running.
            // Once discovered, the app TWI address is used to send the reset command to all devices.
            uint8_t app_addr = twi_bus.ScanBus();
            //NbMicro *micro = new NbMicro(0, SDA, SCL);
            NbMicro *micro = new NbMicro(app_addr, SDA, SCL);
            //micro->SetTwiAddress(app_addr); /* NOTE: All devices share the same TWI application address (44) */
            USE_SERIAL.printf_P("Resetting devices running application at address %d\n\r", micro->GetTwiAddress());
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(1000);
            delete micro;
            Wire.begin(SDA, SCL);
        } else {
            USE_SERIAL.printf_P("\n\rCycle completed %d of %d passes! Letting application run ...\n\n\r", LOOP_COUNT, LOOP_COUNT);
        }
        // for (uint8_t i = 0; i < tml_count; i++) {
        //     delete tml_pool[i];
        // }
    }
    delay(3000);
}

/* _________________
  |                 | 
  |    Main loop    |
  |_________________|
*/
void loop() {
    // Nothing
}

// Determine if there is a user application update available
bool CheckApplUpdate(void) {
    return false;
}

// Function clear screen
void ClrScr() {
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[2J");  // clear screen command
    USE_SERIAL.write(27);     // ESC command
    USE_SERIAL.print("[H");   // cursor to home command
}

// Function PrintLogo
void PrintLogo(void) {
    USE_SERIAL.printf_P("        _                         _\n\r");
    USE_SERIAL.printf_P("    _  (_)                       | |\n\r");
    USE_SERIAL.printf_P("  _| |_ _ ____   ___  ____  _____| |\n\r");
    USE_SERIAL.printf_P(" (_   _) |    \\ / _ \\|  _ \\| ___ | |\n\r");
    USE_SERIAL.printf_P("   | |_| | | | | |_| | | | | ____| |\n\r");
    USE_SERIAL.printf_P("    \\__)_|_|_|_|\\___/|_| |_|_____)\\_)\n\r");
}

// Function print Timonel instance status
Timonel::Status PrintStatus(Timonel *timonel) {
    Timonel::Status tml_status = timonel->GetStatus(); /* Get the instance id parameters received from the ATTiny85 */
    uint8_t twi_address = timonel->GetTwiAddress();
    uint8_t version_major = tml_status.version_major;
    uint8_t version_minor = tml_status.version_minor;
    uint16_t app_start = tml_status.application_start;
    uint8_t app_start_msb = ((tml_status.application_start >> 8) & 0xFF);
    uint8_t app_start_lsb = (tml_status.application_start & 0xFF);
    uint16_t trampoline = ((~(((app_start_lsb << 8) | app_start_msb) & 0xFFF)) + 1);
    trampoline = ((((tml_status.bootloader_start >> 1) - trampoline) & 0xFFF) << 1);
    if ((tml_status.signature == T_SIGNATURE) && ((version_major != 0) || (version_minor != 0))) {
        String version_mj_nick = "";
        switch (version_major) {
            case 0: {
                version_mj_nick = "\"Pre-release\"";
                break;
            }
            case 1: {
                version_mj_nick = "\"Sandra\"";
                break;
            }
            default: {
                version_mj_nick = "\"Unknown\"";
                break;
            }
        }
        USE_SERIAL.printf_P("\n\r Timonel v%d.%d %s ", version_major, version_minor, version_mj_nick.c_str());
        USE_SERIAL.printf_P("(TWI: %02d)\n\r", twi_address);
        USE_SERIAL.printf_P(" ====================================\n\r");
        USE_SERIAL.printf_P(" Bootloader address: 0x%X\n\r", tml_status.bootloader_start);
        if (app_start != 0xFFFF) {
            USE_SERIAL.printf_P("  Application start: 0x%04X (0x%X)\n\r", app_start, trampoline);
        } else {
            USE_SERIAL.printf_P("  Application start: 0x%04X (Not Set)\n\r", app_start);
        }
        USE_SERIAL.printf_P("      Features code: %d | %d ", tml_status.features_code, tml_status.ext_features_code);
        if ((tml_status.ext_features_code >> E_AUTO_CLK_TWEAK) & true) {
            USE_SERIAL.printf_P("(Auto)");
        } else {
            USE_SERIAL.printf_P("(Fixed)");
        }
        USE_SERIAL.printf_P("\n\r");
        USE_SERIAL.printf_P("           Low fuse: 0x%02X\n\r", tml_status.low_fuse_setting);
        USE_SERIAL.printf_P("             RC osc: 0x%02X", tml_status.oscillator_cal);
#if ((defined EXT_FEATURES) && ((EXT_FEATURES >> E_CMD_READDEVS) & true))
        if ((tml_status.ext_features_code >> E_CMD_READDEVS) & true) {
            Timonel::DevSettings dev_settings = timonel->GetDevSettings();
            USE_SERIAL.printf_P("\n\r ....................................\n\r");
            USE_SERIAL.printf_P(" Fuse settings: L=0x%02X H=0x%02X E=0x%02X\n\r", dev_settings.low_fuse_bits, dev_settings.high_fuse_bits, dev_settings.extended_fuse_bits);
            USE_SERIAL.printf_P(" Lock bits: 0x%02X\n\r", dev_settings.lock_bits);
            USE_SERIAL.printf_P(" Signature: 0x%02X 0x%02X 0x%02X\n\r", dev_settings.signature_byte_0, dev_settings.signature_byte_1, dev_settings.signature_byte_2);
            USE_SERIAL.printf_P(" Oscillator: 8.0Mhz=0x%02X, 6.4Mhz=0x%02X", dev_settings.calibration_0, dev_settings.calibration_1);
        }
#endif  // E_CMD_READDEVS
        USE_SERIAL.printf_P("\n\n\r");
    } else {
        USE_SERIAL.printf_P("\n\r *************************************************\n\r");
        USE_SERIAL.printf_P(" * User application running on TWI device %02d ... *\n\r", twi_address);
        USE_SERIAL.printf_P(" *************************************************\n\n\r");
    }
    return tml_status;
}

// Function ThreeStarDelay
void ThreeStarDelay(void) {
    delay(2000);
    for (uint8_t i = 0; i < 3; i++) {
        USE_SERIAL.printf_P("*");
        delay(1000);
    }
}

// Function ShowHeader
void ShowHeader(void) {
    delay(250);
    USE_SERIAL.printf_P("\n\r............................................................\n\r");
    USE_SERIAL.printf_P(". Timonel I2C Bootloader and Application Test (v%d.%d.%d MMS) .\n\r", VER_MAJOR, VER_MINOR, VER_PATCH);
    USE_SERIAL.printf_P("............................................................\n\r");
}
