/*
  =============================================================================
   ESP32 SMART STATION  v2.2  (adds light sensor, buzzer and IR remote)
  =============================================================================
   A desk "smart station" built from the ESP32 starter kit:

     0.96" SSD1306 OLED ........ on-device readout and settings menus
     DHT11 temp / humidity ..... readings, min / max, 20-minute trend graph
     HC-SR501 PIR sensor ....... switches the fan on and off automatically
     KY-023 joystick ........... page through screens, change settings
     DC motor + fan blade ...... PWM speed control through a MOSFET
     Photoresistor ............. dims the OLED in a dark room
     Active buzzer ............. alerts when it gets too hot or too humid
     IR receiver + remote ...... fan and screen control from across the room
     Wi-Fi ..................... web dashboard at http://smartstation.local

   Fan modes (press the joystick on the Home screen to cycle):
     AUTO    the fan runs while motion is detected, then for "Hold time"
     TOGGLE  each new motion event toggles the fan ("wave to switch")
     ON/OFF  manual override

   Pin map and wiring ......... Config.h and ../README.md
   Wi-Fi details .............. secrets.h, or the setup hotspot
   Dashboard source ........... ../dashboard (already built into web_dashboard.h)
   PCB design (Altium) ........ ../docs/PCB_Design_Guide_Altium.md
   Board ...................... "ESP32 Dev Module"  (esp32 by Espressif)
   Partition scheme ........... "Minimal SPIFFS (1.9MB APP with OTA...)"
   Libraries .................. Adafruit SSD1306, Adafruit GFX Library,
                                DHT sensor library, Adafruit Unified Sensor
   Serial monitor ............. 115200 baud, type h for commands
  =============================================================================
*/
#include "Config.h"
#include "AppContext.h"
#include "DisplayUI.h"
#include "WebDashboard.h"

Settings       settings;
FanController  fan;
MotionSensor   motion;
ClimateSensor  climate(PIN_DHT, DHT_TYPE);
Joystick       joystick;
StationWifi network;
DisplayUI      ui;
WebDashboard   web;
FanStatus      fanStatus = {FanReason::WarmingUp, 0, 0};
LightSensor    light;
Buzzer         buzzer;
IrRemote       remote;
AlertStatus    alerts = {false, false};
AppContext     app = {&settings, &fan, &motion, &climate, &joystick, &fanStatus, &network,
                      &light, &buzzer, &remote, &alerts};

// Fan automation state
FanMode  activeMode      = FanMode::Auto;
bool     toggleState     = false;   // Toggle mode: what the last wave chose
bool     fanWasRunning   = false;
bool     motionWasActive = false;
bool     pirWasReady     = false;
uint32_t lastStatusLogMs = 0;
uint32_t lastAlertBeepMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  ESP32 " STATION_NAME "  v" FIRMWARE_VERSION));
  Serial.println(F("========================================"));

  settings.begin();

  if (PIN_STATUS_LED >= 0) {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
  }

  fan.begin(PIN_FAN_PWM);
  motion.begin(PIN_PIR, PIR_WARMUP_MS);
  climate.begin();
  light.begin(PIN_LIGHT);
  buzzer.begin(PIN_BUZZER);
  remote.begin(PIN_IR);
  joystick.begin(PIN_JOY_X, PIN_JOY_Y, PIN_JOY_SW);
  if (!joystick.stickOk()) {
    Serial.println(F("[JOY] stick not detected - check wiring, and leave it centred at power-up"));
  }
  if (!ui.begin(&app)) {
    Serial.println(F("[OLED] no display at 0x3C/0x3D (SDA=D21, SCL=D22) - running without it"));
  }
  network.begin();
  web.begin(&app);

  activeMode = settings.fanMode;
  Serial.printf("[PIR] warming up for %lu s\n", (unsigned long)(PIR_WARMUP_MS / 1000));
  printHelp();
  beep(1, 60, 0, millis());   // power-on chirp
}

void loop() {
  const uint32_t now = millis();

  motion.update(now);
  climate.update(now);
  joystick.update(now);
  light.update(now);
  remote.update(now);

  for (JoyEvent ev = joystick.nextEvent(); ev != JoyEvent::None; ev = joystick.nextEvent()) {
    Serial.printf("[JOY] %s\n", joyEventName(ev));
    ui.handleEvent(ev, now);
  }
  uint8_t remoteKey = 0;
  bool    remoteRepeat = false;
  if (remote.nextKey(remoteKey, remoteRepeat)) handleRemoteKey(remoteKey, remoteRepeat, now);
  handleSerialCommands(now);
  network.update(now);
  web.update();

  updateFanAutomation(now);
  fan.update(now);
  updateAlerts(now);
  buzzer.update(now);

  if (motion.active()) ui.keepAwake(now);   // walking up to the desk wakes the screen
  ui.update(now);
  settings.update(now);

  updateStatusLed(now);
  logChanges(now);
}

// -----------------------------------------------------------------------------
//  The core behaviour: decide whether the fan should run.
// -----------------------------------------------------------------------------
void updateFanAutomation(uint32_t now) {
  const bool newMotionEvent = motion.takeNewEvent();   // consume it every loop
  if (newMotionEvent && settings.motionChime) beep(1, 60, 0, now);

  if (settings.fanMode != activeMode) {
    activeMode = settings.fanMode;
    toggleState = fan.running();   // entering Toggle mode keeps the fan as it is
  }

  bool run = false;
  fanStatus.holdRemainingMs = 0;
  fanStatus.warmupRemainingMs = motion.warmupRemainingMs(now);

  switch (settings.fanMode) {
    case FanMode::On:
      run = true;
      fanStatus.reason = FanReason::ForcedOn;
      break;

    case FanMode::Off:
      run = false;
      fanStatus.reason = FanReason::ForcedOff;
      break;

    case FanMode::Toggle:
      if (!motion.ready()) {
        run = toggleState;
        fanStatus.reason = FanReason::WarmingUp;
        break;
      }
      if (newMotionEvent) toggleState = !toggleState;
      run = toggleState;
      fanStatus.reason = run ? FanReason::ToggledOn : FanReason::ToggledOff;
      break;

    case FanMode::Auto: {
      if (!motion.ready()) {
        run = false;
        fanStatus.reason = FanReason::WarmingUp;
        break;
      }
      const uint32_t holdMs = settings.holdTimeMs();
      const uint32_t sinceMotion = now - motion.lastMotionMs();
      if (motion.active()) {
        run = true;
        fanStatus.reason = FanReason::Motion;
      } else if (motion.everDetected() && sinceMotion < holdMs) {
        run = true;
        fanStatus.reason = FanReason::Hold;
        fanStatus.holdRemainingMs = holdMs - sinceMotion;
      } else {
        run = false;
        fanStatus.reason = FanReason::Idle;
      }

      // Optional temperature gate. 0.5 C of hysteresis stops the fan from
      // chattering when the room sits right at the threshold.
      if (run && settings.tempGateOn && climate.valid()) {
        const float threshold = settings.gateTempC() - (fan.running() ? 0.5f : 0.0f);
        if (climate.temperatureC() < threshold) {
          run = false;
          fanStatus.reason = FanReason::TooCool;
          fanStatus.holdRemainingMs = 0;
        }
      }
      break;
    }
  }

  fan.setSpeedPercent(settings.fanSpeedPct);
  fan.setRunning(run);

  if (run != fanWasRunning) {
    fanWasRunning = run;
    Serial.printf("[FAN] %s (%s)\n", run ? "ON" : "OFF", fanReasonName(fanStatus.reason));
  }
}

// -----------------------------------------------------------------------------
//  Buzzer, climate alerts and the IR remote
// -----------------------------------------------------------------------------
void beep(uint8_t count, uint16_t onMs, uint16_t offMs, uint32_t now) {
  if (settings.soundOn) buzzer.play(count, onMs, offMs, now);
}

// Three beeps when the room first gets too hot or too humid, then two as a
// reminder every ALERT_REPEAT_MS for as long as it stays that way.
void updateAlerts(uint32_t now) {
  if (!settings.climateAlert || !climate.valid()) {
    alerts.tempHigh = false;
    alerts.humidityHigh = false;
    return;
  }

  const float tempLimit = settings.alertTempC();
  const float humLimit = settings.alertHumPct;
  const char* started = nullptr;

  if (!alerts.tempHigh && climate.temperatureC() >= tempLimit) {
    alerts.tempHigh = true;
    started = "Too hot!";
  } else if (alerts.tempHigh && climate.temperatureC() < tempLimit - ALERT_TEMP_HYST_C) {
    alerts.tempHigh = false;
    Serial.println(F("[ALERT] temperature back to normal"));
  }

  if (!alerts.humidityHigh && climate.humidity() >= humLimit) {
    alerts.humidityHigh = true;
    started = "Too humid!";
  } else if (alerts.humidityHigh && climate.humidity() < humLimit - ALERT_HUM_HYST_PCT) {
    alerts.humidityHigh = false;
    Serial.println(F("[ALERT] humidity back to normal"));
  }

  if (started != nullptr) {
    Serial.printf("[ALERT] %s (%.1f C, %.0f %%)\n", started, climate.temperatureC(), climate.humidity());
    lastAlertBeepMs = now;
    beep(3, BEEP_ALERT_ON_MS, BEEP_ALERT_OFF_MS, now);
    ui.keepAwake(now);
    ui.showToast(started, now);
  } else if ((alerts.tempHigh || alerts.humidityHigh) && now - lastAlertBeepMs >= ALERT_REPEAT_MS) {
    lastAlertBeepMs = now;
    beep(2, BEEP_ALERT_ON_MS, BEEP_ALERT_OFF_MS, now);
  }
}

// Fan keys act directly; the arrow and play keys behave like the joystick.
// The key codes are in Config.h.
void handleRemoteKey(uint8_t key, bool repeat, uint32_t now) {
  if (!repeat) Serial.printf("[IR] key 0x%02X\n", key);

  // Only the +/- style keys repeat while held.
  const bool repeats = key == IR_KEY_VOL_UP || key == IR_KEY_VOL_DOWN || key == IR_KEY_UP || key == IR_KEY_DOWN;
  if (repeat && !repeats) return;

  char message[20] = "";
  bool known = true;
  switch (key) {
    case IR_KEY_POWER:    setFanMode(fan.running() ? FanMode::Off : FanMode::On, now); break;
    case IR_KEY_0:        setFanMode(FanMode::Off, now); break;
    case IR_KEY_1:        setFanMode(FanMode::Auto, now); break;
    case IR_KEY_2:        setFanMode(FanMode::Toggle, now); break;
    case IR_KEY_3:        setFanMode(FanMode::On, now); break;
    case IR_KEY_FUNC:
      setFanMode(static_cast<FanMode>((static_cast<uint8_t>(settings.fanMode) + 1) % FAN_MODE_COUNT), now);
      break;
    case IR_KEY_VOL_UP:   changeFanSpeed(FAN_SPEED_STEP_PCT, now); break;
    case IR_KEY_VOL_DOWN: changeFanSpeed(-FAN_SPEED_STEP_PCT, now); break;
    case IR_KEY_PREV:     ui.handleEvent(JoyEvent::Left, now); break;
    case IR_KEY_NEXT:     ui.handleEvent(JoyEvent::Right, now); break;
    case IR_KEY_UP:       ui.handleEvent(JoyEvent::Up, now); break;
    case IR_KEY_DOWN:     ui.handleEvent(JoyEvent::Down, now); break;
    case IR_KEY_PLAY:     ui.handleEvent(JoyEvent::Press, now); break;
    case IR_KEY_EQ:
      settings.soundOn = !settings.soundOn;
      settings.markChanged(now);
      snprintf(message, sizeof(message), "Sound %s", settings.soundOn ? "on" : "off");
      break;
    default:
      known = false;
      break;
  }
  if (!known) return;

  // Fan keys bypass the menus, so show what they did.
  if (key == IR_KEY_VOL_UP || key == IR_KEY_VOL_DOWN) {
    snprintf(message, sizeof(message), "Fan speed %u%%", settings.fanSpeedPct);
  } else if (key == IR_KEY_POWER || key == IR_KEY_FUNC || key == IR_KEY_0 || key == IR_KEY_1 ||
             key == IR_KEY_2 || key == IR_KEY_3) {
    snprintf(message, sizeof(message), "Fan mode: %s", fanModeName(settings.fanMode));
  }
  if (message[0] != '\0') {
    ui.keepAwake(now);
    ui.showToast(message, now);
  }
  if (!repeat) beep(1, BEEP_KEY_MS, 0, now);
}

// On-board LED: blinks while the PIR warms up, then lights while motion is seen.
void updateStatusLed(uint32_t now) {
  if (PIN_STATUS_LED < 0) return;
  const bool on = motion.ready() ? motion.active() : (now / 500) % 2 == 0;
  digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);
}

// -----------------------------------------------------------------------------
//  Serial monitor: logging and simple commands (handy before the OLED or
//  joystick are wired up)
// -----------------------------------------------------------------------------
void logChanges(uint32_t now) {
  if (motion.ready() && !pirWasReady) {
    pirWasReady = true;
    Serial.println(F("[PIR] warm-up complete, watching for motion"));
  }
  if (motion.active() != motionWasActive) {
    motionWasActive = motion.active();
    if (motionWasActive) Serial.printf("[PIR] motion detected (event #%lu)\n", (unsigned long)motion.eventCount());
    else                 Serial.println(F("[PIR] motion ended"));
  }
  if (now - lastStatusLogMs >= STATUS_LOG_INTERVAL_MS) {
    lastStatusLogMs = now;
    printStatus(now);
  }
}

void printStatus(uint32_t now) {
  char temp[12] = "--";
  char hum[8] = "--";
  if (climate.valid()) {
    snprintf(temp, sizeof(temp), "%.1fC", climate.temperatureC());
    snprintf(hum, sizeof(hum), "%.0f%%", climate.humidity());
  }
  const char* motionText = !motion.ready() ? "warming up" : (motion.active() ? "YES" : "no");
  Serial.printf("[STATUS] up %lus | T %s H %s | light %u%% | motion %s | fan %s %u%% (duty %u) | mode %s, %s\n",
                (unsigned long)(now / 1000), temp, hum, light.percent(), motionText,
                fan.running() ? "ON" : "OFF", settings.fanSpeedPct, fan.duty(),
                fanModeTag(settings.fanMode), fanReasonName(fanStatus.reason));
}

void printHelp() {
  Serial.println(F("Serial commands:"));
  Serial.println(F("  s  status          a  fan AUTO       t  fan TOGGLE"));
  Serial.println(F("  1  fan ON          0  fan OFF        +/-  fan speed"));
  Serial.println(F("  r  reset stats     c  centre stick   w  Wi-Fi status"));
  Serial.println(F("  b  buzzer test     l  light reading  h  this help"));
}

void printWifiStatus() {
  if (network.state() == NetState::Connected) {
    Serial.printf("[WIFI] connected to \"%s\" | http://%s | http://%s.local | %d dBm\n",
                  network.stationSsid().c_str(), network.ip().c_str(), WIFI_HOSTNAME, network.rssi());
  } else if (network.state() == NetState::Connecting) {
    Serial.printf("[WIFI] connecting to \"%s\"\n", network.stationSsid().c_str());
  }
  if (network.hotspotActive()) {
    Serial.printf("[WIFI] hotspot \"%s\" | http://%s | %u client(s)\n",
                  network.apSsid().c_str(), network.apIp().c_str(), network.hotspotClients());
  }
}

void setFanMode(FanMode mode, uint32_t now) {
  settings.fanMode = mode;
  settings.markChanged(now);
  Serial.printf("[CMD] fan mode %s\n", fanModeTag(mode));
}

void changeFanSpeed(int delta, uint32_t now) {
  settings.fanSpeedPct = constrain(settings.fanSpeedPct + delta, FAN_SPEED_MIN_PCT, FAN_SPEED_MAX_PCT);
  settings.markChanged(now);
  Serial.printf("[CMD] fan speed %u%%\n", settings.fanSpeedPct);
}

void handleSerialCommands(uint32_t now) {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    switch (c) {
      case 'h':
      case '?': printHelp(); break;
      case 's': printStatus(now); break;
      case 'w': printWifiStatus(); break;
      case 'a': setFanMode(FanMode::Auto, now); break;
      case 't': setFanMode(FanMode::Toggle, now); break;
      case '1': setFanMode(FanMode::On, now); break;
      case '0': setFanMode(FanMode::Off, now); break;
      case '+': changeFanSpeed(FAN_SPEED_STEP_PCT, now); break;
      case '-': changeFanSpeed(-FAN_SPEED_STEP_PCT, now); break;
      case 'r':
        climate.resetStats();
        motion.resetCount();
        fan.resetRuntime();
        Serial.println(F("[CMD] stats cleared"));
        break;
      case 'b':
        buzzer.play(2, BEEP_ALERT_ON_MS, BEEP_ALERT_OFF_MS, now);   // sounds even when Sound is off
        Serial.println(F("[CMD] buzzer test"));
        break;
      case 'l':
        Serial.printf("[LIGHT] %u%% (raw %d of 4095)%s\n", light.percent(), light.raw(), light.dark() ? ", dark" : "");
        break;
      case 'c':
        Serial.println(joystick.calibrate() ? F("[CMD] stick centred") : F("[CMD] stick not found"));
        break;
      default:
        break;   // ignore line endings and unknown keys
    }
  }
}
