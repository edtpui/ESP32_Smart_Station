# ESP32 Smart Station

An ESP32 desk station with an OLED, DHT11 temperature and humidity sensor, PIR motion sensor, joystick, and a DC-motor fan that motion switches on and off. A light sensor dims the OLED in the dark, a buzzer warns when the room gets too hot or humid, and an IR remote controls the fan from across the room. A Wi-Fi dashboard shows the same readings and controls from any browser.

```
ESP32_Smart_Station/
├─ ESP32_Smart_Station/   Arduino sketch: open the .ino in the Arduino IDE
├─ dashboard/             Dashboard source (React, shadcn/ui, Tailwind)
└─ docs/                  wiring_diagram.svg and PCB_Design_Guide_Altium.md
```

## Upload the firmware

The ESP32 board package and all libraries are already installed in this PC's Arduino IDE. On another PC, install **esp32 by Espressif Systems** from the Boards Manager, and **Adafruit SSD1306** and **DHT sensor library** from the Library Manager. Accept the "install all" prompt so their dependencies come too.

1. Open `ESP32_Smart_Station/ESP32_Smart_Station.ino`.
2. **Tools > Board > esp32 > ESP32 Dev Module**.
3. **Tools > Partition Scheme > Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**. This is required: the firmware is 1.42 MB and the default partition holds 1.31 MB, so anything else fails with "Sketch too big".
4. **Tools > Port**: pick your board, then click Upload. If it stalls on `Connecting...`, hold the board's BOOT button until writing starts.
5. Open the Serial Monitor at 115200 baud. Type `h` for commands.

## Connect to Wi-Fi

Use either method. You don't need to re-upload for the second one.

**Method 1: `secrets.h`.** Fill in `WIFI_SSID` and `WIFI_PASSWORD`, then upload. Leave the two `WIFI_EAP_` lines empty unless your network has a username login.

**Method 2: the setup hotspot.** With no network set, or when the network can't be reached for 20 s, the station opens its own Wi-Fi network:

1. The OLED's **Wi-Fi** page shows its name (`SmartStation-XXXX`) and password. The default password is `smartstation`; change it in `secrets.h`.
2. Join that network from a phone or laptop and open `http://192.168.4.1`.
3. Under **Wi-Fi**, click **Change network**, pick your network, and enter its details.

Once connected, the OLED's **Wi-Fi** page shows the address. Open that, or `http://smartstation.local`.

### Campus Wi-Fi and eduroam

Username logins (WPA2-Enterprise PEAP) are supported, with limits you should know before using them:

- **Your login is stored unencrypted on the chip,** and the station doesn't verify the network's certificate. Don't use your main university password. Ask IT whether they offer a device network or device login.
- **Campus networks usually block device-to-device traffic,** so your laptop may not reach the dashboard even when the station is online. A home network or phone hotspot avoids this.

### Security

The dashboard uses plain HTTP, and it has no login until you set `DASHBOARD_PASSWORD` in `secrets.h`. Use it on networks you trust. `secrets.h` is listed in `.gitignore`, so don't remove that line or share the file.

## Wiring

See [docs/wiring_diagram.svg](docs/wiring_diagram.svg) for the picture. It predates the light sensor, buzzer and IR receiver, which are in the tables below.

| Module | Module pin | Connect to | Note |
|---|---|---|---|
| OLED SSD1306 | GND, VCC | GND, **3V3** | Some modules swap GND and VCC: check the labels |
| | SCL, SDA | D22, D21 | |
| DHT11 | VCC, GND | **3V3**, GND | |
| | DATA | D25 | Add a 10 kΩ resistor to 3V3 if yours is a bare sensor |
| PIR HC-SR501 | VCC, GND | **VIN**, GND | Needs 5 V; its output is 3.3 V, so it is safe |
| | OUT | D33 | |
| Joystick KY-023 | GND, +5V | GND, **3V3** | Use 3V3 even though the pin says +5V |
| | VRx, VRy, SW | D34, D35, D32 | |
| Photoresistor | One leg | **3V3** | Either way round |
| | Other leg | VP (GPIO36) | Also put a 10 kΩ resistor from VP to GND |
| IR receiver module | G, R | GND, **3V3** | G is GND, R is power, Y is the signal |
| | Y | D14 | |
| Active buzzer | + (long leg) | **VIN** | Driven through a transistor, see below |

**Never power the OLED, DHT11 or joystick from 5 V.** Their signal lines would then reach the ESP32 at 5 V, and its pins aren't 5 V tolerant.

**PIR adjustments:** turn the Time pot fully anti-clockwise and set the jumper to H (repeat trigger), so the dashboard's Hold time is what you get.

### Fan driver

An ESP32 pin can't drive a motor directly. D26 works with any of these:

| Driver | Wiring |
|---|---|
| MOSFET (the PCB design) | See the fan driver schematic in the wiring diagram |
| L293D chip | D26 to EN1 (pin 1), 5 V to IN1 (pin 2), GND to IN2 (pin 7), motor on OUT1 and OUT2 (pins 3, 6), 5 V to pins 8 and 16, GND to pins 4, 5, 12, 13 |
| L9110 fan module | D26 to INA, GND to INB, plus VCC and GND |

### Buzzer driver

The buzzer runs from 5 V through a PN2222 transistor, so D27 only has to supply a small base current. Hold the PN2222 with its flat face towards you and legs down: the legs are emitter, base, collector from left to right.

| From | To |
|---|---|
| D27 | 1 kΩ resistor, then the PN2222's base (middle leg) |
| PN2222 emitter (left leg) | GND |
| PN2222 collector (right leg) | Buzzer − (short leg) |
| Buzzer + (long leg) | VIN |
| 1N5819 diode | Across the buzzer, with the band towards VIN |

Use the **active** buzzer, the one with a sealed underside and a sticker on top. The passive one (open underside showing a small circuit board) only clicks.

## Using it

| Fan mode | Behaviour |
|---|---|
| **Auto** (default) | Runs while motion is detected, then for the Hold time |
| **Toggle** | Each new motion event flips the fan on or off |
| **On / Off** | Manual override |

| Joystick | Action |
|---|---|
| Left / Right | Change page: Home, Fan, Trend, Wi-Fi, System, Info |
| Up / Down | Move the cursor. On Home: fan speed |
| Press | Edit or confirm. On Home: next fan mode. On Wi-Fi: hotspot on or off |
| Long press | Back to Home. On Factory reset: reset settings |

| Remote key | Action |
|---|---|
| Power | Fan on or off |
| VOL+ / VOL− | Fan speed. Hold to keep changing |
| FUNC/STOP | Next fan mode |
| 0, 1, 2, 3 | Fan Off, Auto, Toggle, On |
| ⏮ / ⏭ | Previous or next page |
| ▲ / ▼ and ⏯ | Same as joystick up, down and press |
| EQ | Sound on or off |

The Serial Monitor prints the code of every key it receives (`[IR] key 0x45`). If your remote's keys do something else, put the codes it prints into the `IR_KEY_` lines in `Config.h`.

The buzzer beeps three times when the temperature or humidity passes its alert limit (30 °C and 70 % by default), then twice every 5 minutes until the room recovers. Change the limits, or switch on a beep for every motion event, under **Sound and alerts** on the dashboard or in the OLED's System menu. With **Auto dim** on, the OLED drops to Low brightness while the light sensor reads dark.

The PIR needs about 30 s to settle after power-up, so Auto mode waits until then. The OLED sleeps after 60 s without motion or joystick use (adjustable); either one wakes it. Settings survive power cuts and re-uploads.

If joystick directions come out wrong, change `JOY_SWAP_XY`, `JOY_INVERT_X` or `JOY_INVERT_Y` in `Config.h`. The serial log prints each direction it detects, which makes this easy to check. The other hardware options are in `Config.h` too, such as `DHT_TYPE` for a DHT22 and `FAN_MIN_DUTY` for a motor that won't start at low speed.

## Change the dashboard

The firmware already contains the built page (`web_dashboard.h`), so you only need Node.js to change it. Run these in `dashboard/`:

```bash
npm install
```

Build the page and try it at http://localhost:8787 against a simulated station, with no hardware needed:

```bash
npm run build
```

```bash
npm run mock
```

Regenerate `web_dashboard.h`, then upload the sketch again:

```bash
npm run build:esp32
```

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| "Sketch too big" | Partition Scheme isn't set to Minimal SPIFFS |
| Resets when the fan starts, serial shows `Brownout detector` | Weak USB power. Use a 5 V charger rated 1 A or more, and a short cable |
| Temperature shows `--` | DHT11 wiring, or a bare sensor without its pull-up resistor |
| Temperature reads high | Heat from the ESP32 reaches the sensor. Move it away, or set `TEMP_OFFSET_C` |
| Buzzer only clicks, or is silent | It's the passive buzzer, or it's in backwards. Type `b` in the Serial Monitor to test it |
| Buzzer never stops | The transistor's legs are in the wrong order |
| Light reads 0 % or jumps around | The 10 kΩ resistor from VP to GND is missing, or the photoresistor isn't on 3V3. Type `l` to see the reading |
| OLED dims by itself | Auto dim. The light sensor reads dark: switch it off in System, or lower `LIGHT_DARK_ON_PCT` |
| Remote does nothing | Aim at the receiver's dome and check the serial log for `[IR] key`. No lines at all means wiring; lines with other codes mean a different remote |
| Fan runs at random | PIR still settling, or too sensitive: lower its Sens pot |
| OLED stays blank | Check SDA and SCL, and whether GND and VCC are swapped. Serial reports `[OLED] no display` |
| `smartstation.local` won't open | Some networks block it. Use the IP address from the OLED |
| Dashboard unreachable on campus Wi-Fi | Device-to-device traffic is blocked. Use a home network, phone hotspot, or the station's hotspot |
| New `secrets.h` values ignored | A network saved from the dashboard wins. Click **Forget saved network** |
