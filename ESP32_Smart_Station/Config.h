#pragma once
// =============================================================================
//  ESP32 Smart Station - Config.h
//
//  Every pin, timing and default value lives in this one file. If something
//  about your hardware is different (sensor type, joystick orientation, a fan
//  motor that needs more power to start...), change it here.
// =============================================================================
#include <Arduino.h>

#define STATION_NAME     "Smart Station"
#define FIRMWARE_VERSION "2.2.0"

// -----------------------------------------------------------------------------
//  PIN MAP  -  ESP32 DevKit V1 (30-pin, ESP32-WROOM-32)
//
//  New layout: every module signal comes off the LEFT header in one block
//  (D34 ... D26), and the OLED uses the ESP32's default I2C pins (D21 / D22)
//  on the RIGHT header. Grouping the pins like this keeps PCB traces short and
//  stops them from crossing each other.
//
//   Silkscreen  GPIO  Signal      Goes to
//   ----------  ----  ----------  ---------------------------------------------
//   D34         34    JOY_X       Joystick VRx  (analog in, ADC1)
//   D35         35    JOY_Y       Joystick VRy  (analog in, ADC1)
//   D32         32    JOY_SW      Joystick SW   (shorts to GND when pressed)
//   D33         33    PIR_OUT     PIR OUT       (HIGH = motion)
//   D25         25    DHT_DATA    DHT11 DATA    (10k pull-up to 3V3)
//   D26         26    FAN_PWM     100R -> MOSFET gate (100k pull-down to GND)
//   D27         27    BUZZ        1k -> PN2222 base; the transistor switches the buzzer
//   D14         14    IR_OUT      IR receiver OUT (idles HIGH, pulses LOW)
//   VP          36    LIGHT       photoresistor to 3V3, 10k to GND (analog in, ADC1)
//   D21         21    I2C_SDA     OLED SDA
//   D22         22    I2C_SCL     OLED SCL
//   D2          2     STATUS_LED  on-board blue LED (nothing to wire)
//   VIN         -     +5V         PIR VCC, fan motor (+), buzzer (+)
//   3V3         -     +3V3        OLED, DHT11, joystick, IR receiver and photoresistor
//
//  Why these pins:
//   * The joystick axes and the photoresistor are on ADC1 (GPIO32-39). ADC2
//     stops working when Wi-Fi is on, so ADC2 pins would lose them as soon as
//     the dashboard runs.
//   * No strapping pins (GPIO0, 2, 5, 12, 15) have external parts on them, so
//     nothing plugged in can stop the board from booting or uploading.
//   * GPIO26 stays quiet during boot (GPIO5, 14 and 15 pulse), so the fan never
//     twitches when the board resets. GPIO14 is only ever an input here, so its
//     boot-time pulse can't switch anything on.
// -----------------------------------------------------------------------------
constexpr int PIN_JOY_X      = 34;
constexpr int PIN_JOY_Y      = 35;
constexpr int PIN_JOY_SW     = 32;
constexpr int PIN_PIR        = 33;
constexpr int PIN_DHT        = 25;
constexpr int PIN_FAN_PWM    = 26;
constexpr int PIN_BUZZER     = 27;
constexpr int PIN_IR         = 14;
constexpr int PIN_LIGHT      = 36;      // the pin labelled VP
constexpr int PIN_I2C_SDA    = 21;
constexpr int PIN_I2C_SCL    = 22;
constexpr int PIN_STATUS_LED = 2;       // -1 if your board has no LED on GPIO2

// ----- OLED (SSD1306, 128x64, I2C) -------------------------------------------
constexpr int      OLED_WIDTH       = 128;
constexpr int      OLED_HEIGHT      = 64;
constexpr uint8_t  OLED_I2C_ADDRESS = 0x3C;    // 0x3D is tried automatically
constexpr uint32_t I2C_CLOCK_HZ     = 400000;

// ----- Temperature / humidity sensor -----------------------------------------
constexpr uint8_t  DHT_TYPE                 = 11;     // 11 = DHT11 (blue), 22 = DHT22 / AM2302 (white)
constexpr uint32_t CLIMATE_READ_INTERVAL_MS = 2500;   // DHT11: max 1 read/s, DHT22: 1 read/2 s
constexpr uint32_t CLIMATE_STALE_MS         = 10000;  // show "--" after this long without a good read
constexpr float    TEMP_OFFSET_C            = 0.0f;   // calibration, added to every reading
constexpr float    HUMIDITY_OFFSET_PCT      = 0.0f;
constexpr uint8_t  TREND_HISTORY_LEN        = 100;    // points on the trend graph
constexpr uint32_t TREND_SAMPLE_INTERVAL_MS = 12000;  // 100 x 12 s = last 20 minutes

// ----- PIR motion sensor (HC-SR501) ------------------------------------------
// Set the PIR's "Time" pot fully anti-clockwise (shortest) and its jumper to
// "H" (repeat trigger) so the Hold time in the Fan menu is what you get.
constexpr uint32_t PIR_WARMUP_MS   = 30000;   // output is unreliable for ~30-60 s after power-up
constexpr uint32_t PIR_DEBOUNCE_MS = 50;

// ----- Joystick (KY-023 style) -----------------------------------------------
// If a direction comes out wrong, flip these instead of rewiring.
constexpr bool     JOY_SWAP_XY          = false;
constexpr bool     JOY_INVERT_X         = false;
constexpr bool     JOY_INVERT_Y         = false;
constexpr int      JOY_THRESHOLD_ON     = 1100;   // ADC counts from centre that count as a push
constexpr int      JOY_THRESHOLD_OFF    = 600;    // must come back inside this to release
constexpr uint32_t JOY_SAMPLE_MS        = 20;
constexpr uint32_t JOY_REPEAT_DELAY_MS  = 450;    // hold a direction: first repeat after...
constexpr uint32_t JOY_REPEAT_RATE_MS   = 150;    // ...then one step every...
constexpr uint32_t BUTTON_DEBOUNCE_MS   = 30;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 800;

// ----- Fan (DC motor through a MOSFET, or an L293D / L9110 driver) -----------
constexpr uint32_t FAN_PWM_FREQ_HZ  = 20000;  // above human hearing = no motor whine
constexpr uint8_t  FAN_PWM_BITS     = 8;      // duty 0-255
constexpr uint8_t  FAN_PWM_CHANNEL  = 0;      // only used with ESP32 core 2.x
constexpr uint8_t  FAN_MIN_DUTY     = 90;     // lowest duty your motor reliably starts at
constexpr uint32_t FAN_SOFTSTART_MS = 700;    // 0 -> full ramp time, avoids USB brown-outs

// ----- Light sensor (photoresistor + 10k divider) ------------------------------
// The reading rises with brightness: 0 % is dark, 100 % is direct light.
constexpr uint32_t LIGHT_SAMPLE_MS   = 200;
constexpr uint8_t  LIGHT_DARK_ON_PCT  = 12;   // below this the room counts as dark...
constexpr uint8_t  LIGHT_DARK_OFF_PCT = 20;   // ...until it rises above this again

// ----- Buzzer (active type: sounds whenever its pin is HIGH) -------------------
constexpr uint16_t BEEP_KEY_MS       = 25;      // click for each remote key
constexpr uint16_t BEEP_ALERT_ON_MS  = 120;
constexpr uint16_t BEEP_ALERT_OFF_MS = 120;
constexpr uint32_t ALERT_REPEAT_MS   = 300000;  // reminder while an alert lasts (5 min)
constexpr float    ALERT_TEMP_HYST_C = 0.5f;    // an alert clears this far below its limit
constexpr float    ALERT_HUM_HYST_PCT = 3.0f;

// ----- IR remote (NEC protocol, the 21-key remote from the kit) ----------------
// Every key received is printed on the Serial Monitor as "[IR] key 0x45". If
// your remote sends different codes, press each key and put its code here.
constexpr uint8_t IR_KEY_POWER      = 0x45;   // fan on / off
constexpr uint8_t IR_KEY_VOL_UP     = 0x46;   // fan speed +
constexpr uint8_t IR_KEY_VOL_DOWN   = 0x15;   // fan speed -
constexpr uint8_t IR_KEY_FUNC       = 0x47;   // next fan mode
constexpr uint8_t IR_KEY_PREV       = 0x44;   // previous page
constexpr uint8_t IR_KEY_NEXT       = 0x43;   // next page
constexpr uint8_t IR_KEY_PLAY       = 0x40;   // same as pressing the joystick
constexpr uint8_t IR_KEY_UP         = 0x09;   // same as joystick up
constexpr uint8_t IR_KEY_DOWN       = 0x07;   // same as joystick down
constexpr uint8_t IR_KEY_EQ         = 0x19;   // sound on / off
constexpr uint8_t IR_KEY_0          = 0x16;   // fan Off
constexpr uint8_t IR_KEY_1          = 0x0C;   // fan Auto
constexpr uint8_t IR_KEY_2          = 0x18;   // fan Toggle
constexpr uint8_t IR_KEY_3          = 0x5E;   // fan On
constexpr uint32_t IR_REPEAT_DELAY_MS = 450;  // hold a key: first repeat after...
constexpr uint32_t IR_REPEAT_RATE_MS  = 200;  // ...then one step every...

// ----- Defaults for the settings you can change from the OLED menu ------------
// (saved in flash; "Factory reset" in the System menu brings these back)
constexpr uint8_t  DEFAULT_FAN_SPEED_PCT = 80;
constexpr uint8_t  DEFAULT_HOLD_INDEX    = 3;     // 30 s, see HOLD_TIME_OPTIONS_S in Settings.h
constexpr int16_t  DEFAULT_GATE_TEMP_C10 = 260;   // 26.0 C, stored in tenths of a degree
constexpr uint8_t  DEFAULT_SLEEP_INDEX   = 3;     // 60 s, see SCREEN_SLEEP_OPTIONS_S
constexpr uint8_t  DEFAULT_BRIGHTNESS    = 2;     // 0 low, 1 medium, 2 high
constexpr int16_t  DEFAULT_ALERT_TEMP_C10 = 300;  // 30.0 C
constexpr uint8_t  DEFAULT_ALERT_HUM_PCT  = 70;

// ----- Wi-Fi and web dashboard -------------------------------------------------
// Your network name and password go in secrets.h, or set them from the dashboard.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID          ""
#define WIFI_PASSWORD      ""
#define WIFI_EAP_USERNAME  ""
#define WIFI_EAP_IDENTITY  ""
#define SETUP_AP_PASSWORD  "smartstation"
#define DASHBOARD_USERNAME "admin"
#define DASHBOARD_PASSWORD ""
#endif

#define WIFI_HOSTNAME "smartstation"                 // http://smartstation.local
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;  // then the setup hotspot opens
constexpr uint32_t WIFI_RETRY_INTERVAL_MS  = 60000;  // how often the hotspot retries the router
constexpr uint32_t WIFI_APPLY_DELAY_MS     = 1500;   // lets the HTTP reply go out before switching
constexpr uint16_t WEB_PORT                = 80;

// ----- Misc --------------------------------------------------------------------
constexpr uint32_t UI_FRAME_MS            = 100;    // OLED refresh (10 fps)
constexpr uint32_t SPLASH_MS              = 1500;
constexpr uint32_t TOAST_MS               = 1300;   // pop-up message duration
constexpr uint32_t SETTINGS_SAVE_DELAY_MS = 1500;   // batch flash writes while you scroll values
constexpr uint32_t STATUS_LOG_INTERVAL_MS = 10000;  // Serial status line
constexpr uint32_t SERIAL_BAUD            = 115200;
