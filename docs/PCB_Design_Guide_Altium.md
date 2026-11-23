# Smart Station PCB in Altium Designer

A first-board walkthrough. You'll design a 2-layer **carrier board**: the ESP32 DevKit plugs into two header rows, and each module connects through its own header.

Putting a bare ESP32 chip on the board would also mean designing USB, power regulation, auto-reset and an RF antenna layout. The DevKit already has all of that, so this design leaves only one small fan driver circuit to get right.

> **How far this guide is verified:** the menu names come from Altium's documentation and JLCPCB's export guide (Altium Designer 24 to 26), but they haven't been checked step by step in Altium. A name may differ slightly in your version. The netlist matches the pin map in `Config.h`, and that circuit and firmware have worked on a breadboard: OLED, DHT11, joystick, motion-switched MOSFET fan and Wi-Fi dashboard. The PCB itself is still unproven, so follow the checks in section 15 before ordering.

## 1. Measure your DevKit first

This guide assumes a 30-pin **DOIT ESP32 DevKit V1**. Clones vary, and a wrong footprint means a board your DevKit won't fit.

- **Row spacing:** **25.4 mm (1.0 in)** on this project's board, measured centre to centre between the two pin rows. Boards of this type also come in 22.86 mm and 27.94 mm. The first board run used 22.86 mm and the DevKit didn't fit, so measure with calipers or a ruler before you make the footprint, and again if you ever change boards.
- **Pin order:** compare the labels on your board with the table in section 3.
- **Board outline:** measure length and width. You'll draw this outline on the footprint in section 5.

If you have a 38-pin board, the GPIO numbers still work, but the footprint and pin numbers below don't.

## 2. Parts list

| Ref | Part | Value / spec | Notes |
|---|---|---|---|
| U1 | ESP32 DevKit V1 | 30-pin | Plugs in, not soldered |
| – | Female header × 2 | 1×15, 2.54 mm | Soldered to the board, holds U1 |
| J1 | Header | 1×4, 2.54 mm | OLED |
| J2 | Header (optional) | 1×4, 2.54 mm | I2C expansion for more sensors later |
| J3 | Header | 1×3, 2.54 mm | DHT11 |
| J4 | Header | 1×3, 2.54 mm | PIR |
| J5 | Header | 1×5, 2.54 mm | Joystick |
| J6 | Screw terminal | 2-way, 5.08 mm | Fan motor, e.g. Phoenix Contact MKDS 1,5/2-5,08 |
| J7 | Header | 1×2, 2.54 mm | Photoresistor |
| J8 | Header footprint | 1×4, 2.54 mm | Active buzzer. Its legs are 7.62 mm apart, so they go in holes 1 and 4 |
| J9 | Header | 1×3, 2.54 mm | IR receiver module |
| Q2 | NPN transistor | PN2222A, TO-92 | Switches the buzzer |
| D2 | Schottky diode | 1N5819, DO-41 | Across the buzzer |
| Q1 | N-MOSFET | AO3400A, SOT-23 | Fully on at 3.3 V gate drive. The only surface-mount part |
| D1 | Schottky diode | 1N5819, DO-41 | Absorbs the motor's voltage spike at switch-off |
| R1 | Resistor | 100 Ω, ¼ W | Gate series resistor |
| R2 | Resistor | 100 kΩ, ¼ W | Holds the fan off during reset |
| R3 | Resistor | 10 kΩ, ¼ W | DHT11 data pull-up |
| R4 | Resistor | 1 kΩ, ¼ W | LED1 current limit |
| R5 | Resistor | 10 kΩ, ¼ W | Lower half of the light sensor divider |
| R6 | Resistor | 1 kΩ, ¼ W | Q2 base resistor |
| LED1 | LED | Green, 3 mm | Lights when Q1 switches the fan on |
| C1 | Electrolytic | 470 µF, 16 V, radial, 8 mm wide, 3.5 mm lead spacing | Supplies the motor's start-up current |
| C2, C3 | Ceramic | 100 nF, 50 V, radial, 2.54 mm lead spacing | Decoupling on +5V and +3V3 |
| H1–H4 | Mounting hole | M3 (3.2 mm) | Corners |

The 2.54 mm headers can be male or female, and the footprint is the same either way. Decide when you solder: use female to plug a module straight in, male for jumper wires.

## 3. Netlist

Your schematic must produce exactly these connections. Use this table to check it.

| Net | Connects |
|---|---|
| `+5V` | U1.15 (VIN), J4.1, J6.1, J8.1, D1 cathode, D2 cathode, R4.1, C1 +, C2.1 |
| `+3V3` | U1.16 (3V3), J1.2, J2.2, J3.1, J5.2, J7 (one pin), J9.2, R3.1, C3.1 |
| `GND` | U1.14, U1.17, J1.1, J2.1, J3.3, J4.3, J5.1, J9.1, Q1.2 (S), Q2 emitter, R2.2, R5 (one end), C1 −, C2.2, C3.2 |
| `I2C_SDA` | U1.26 (D21), J1.4, J2.4 |
| `I2C_SCL` | U1.29 (D22), J1.3, J2.3 |
| `JOY_X` | U1.4 (D34), J5.3 |
| `JOY_Y` | U1.5 (D35), J5.4 |
| `JOY_SW` | U1.6 (D32), J5.5 |
| `PIR_OUT` | U1.7 (D33), J4.2 |
| `DHT_DATA` | U1.8 (D25), J3.2, R3.2 |
| `FAN_PWM` | U1.9 (D26), R1.2, R2.1 |
| `FAN_GATE` | R1.1, Q1.1 (G) |
| `FAN_NEG` | Q1.3 (D), J6.2, D1 anode, LED1 cathode |
| `LED_A` | R4.2, LED1 anode |
| `LIGHT` | U1.2 (VP), J7 (other pin), R5 (other end) |
| `BUZZ` | U1.10 (D27), R6 (one end) |
| `BUZZ_BASE` | R6 (other end), Q2 base |
| `BUZZ_NEG` | Q2 collector, J8.4, D2 anode |
| `IR_OUT` | U1.11 (D14), J9.3 |

**Connector pin order** (pin 1 first). Check these against your modules' labels, and swap pins here if yours differ:

| Connector | Pins |
|---|---|
| J1, J2 | GND, +3V3, SCL, SDA |
| J3 | +3V3, DATA, GND |
| J4 | +5V, OUT, GND |
| J5 | GND, +3V3, VRx, VRy, SW |
| J6 | FAN+ (+5V), FAN− |
| J7 | +3V3, LIGHT (a photoresistor works either way round) |
| J8 | Buzzer + (+5V), unused, unused, Buzzer − |
| J9 | GND, +3V3, OUT |

**U1 pin numbers.** These follow chip-style numbering so Altium's footprint wizard can generate the footprint: down the left side, then up the right, seen from above with the antenna at the top.

| Pin | Label | | Pin | Label |
|---|---|---|---|---|
| 1 | EN | | 30 | D23 |
| 2 | **VP** | | 29 | **D22** |
| 3 | VN | | 28 | TX0 |
| 4 | **D34** | | 27 | RX0 |
| 5 | **D35** | | 26 | **D21** |
| 6 | **D32** | | 25 | D19 |
| 7 | **D33** | | 24 | D18 |
| 8 | **D25** | | 23 | D5 |
| 9 | **D26** | | 22 | TX2 |
| 10 | **D27** | | 21 | RX2 |
| 11 | **D14** | | 20 | D4 |
| 12 | D12 | | 19 | D2 |
| 13 | D13 | | 18 | D15 |
| 14 | **GND** | | 17 | **GND** |
| 15 | **VIN** | | 16 | **3V3** |

## 4. Create the project

1. **File » New » Project**. Choose **PCB**, then the empty template. Name it `Smart_Station` and save it in `C:\Arduino\ESP32_Smart_Station\hardware`. Untick **Constraint Manager**: section 10 uses **Design » Rules**, which isn't available with it on, and a project can't be switched back afterwards.
2. In the **Projects** panel, right-click the project and use **Add New to Project** to add a **Schematic**, a **PCB**, a **Schematic Library** and a **PCB Library**.
3. Save each one (**File » Save As**) with a sensible name. Save often: Altium doesn't autosave everything for you.

The basic pieces are: a *symbol* is the schematic drawing of a part, a *footprint* is its copper pattern on the board, and a *component* links the two.

## 5. Make the ESP32 footprint

Open the PCB Library, then **Tools » Footprint Wizard**.

1. Choose **Dual In-line Packages (DIP)**, with units in **mm**.
2. Pads: **1.7 mm** diameter, **1.0 mm** hole.
3. Spacing: **2.54 mm** along a row and **25.4 mm** between rows (use your own measurement from section 1).
4. Outline width 0.2 mm, **30 pads**, name `ESP32_DEVKIT_V1_30P`.
5. Finish. Then delete the wizard's outline and draw the real board outline on **Top Overlay** using your measurements. This project's DevKit measured 51 × 28 mm, with EN 6 mm from the antenna end and VIN 10 mm from the USB end, so with pad 1 at the origin the outline's corners are (-2.57, 6) and (25.43, -45.56). Add the text `ANTENNA` at the pin 1 end and `USB` at the other, so nobody can plug it in backwards.

## 6. Make the ESP32 symbol

Open the Schematic Library. In the **SCH Library** panel, name the component `ESP32_DEVKIT_V1_30P`.

1. **Place » Rectangle** to draw the body.
2. **Place » Pin**. Press **Tab** while a pin is on the cursor, then set its **Designator** (the pin number) and **Name** (the label) from the table in section 3. The dot end of a pin must face outwards, because that's where wires connect.
3. Place all 30 pins with **Electrical Type: Passive**, so Altium doesn't make assumptions about a module it can't see inside.
4. In the **Properties** panel, under **Footprint**, click **Add** and choose `ESP32_DEVKIT_V1_30P`.

## 7. Draw the schematic

Open **Panels** (bottom-right of the window) and turn on **Components**. Find parts here:

- **Miscellaneous Connectors:** `Header 3`, `Header 4`, `Header 5`.
- **Miscellaneous Devices:** resistors (`Res2`), ceramic capacitors (`Cap`). If these libraries aren't listed, install them from `C:\Users\Public\Documents\Altium\<version>\Library`.
- **Manufacturer Part Search** panel: search `AO3400A`, `1N5819`, the screw terminal, an LED, and a 470 µF 16 V radial capacitor. Picking a real part brings a correct footprint with it.

Then build the circuit:

1. **Place » Part** for each component. **Space** rotates, **Tab** edits while placing.
2. **Place » Power Port** for `+5V`, `+3V3` and `GND`. Ports with the same name are connected automatically.
3. **Place » Net Label** for the signal nets in section 3 (like `JOY_X`). The same label on two wires connects them, which keeps the drawing clean. Add a short wire to each pin, then a label on that wire.
4. **Place » Wire** for the fan driver, drawn as in `wiring_diagram.svg`. Watch D1's direction: the cathode (the band) goes to +5V.
5. Check each part's footprint in **Properties**: resistors `AXIAL-0.4` (10.16 mm), C2 and C3 with 2.54 mm spacing, headers `HDR1X3`, `HDR1X4`, `HDR1X5`. Change any that don't fit your physical parts.
6. **Tools » Annotation » Annotate Schematics** numbers your parts. Set the designators by hand if you want them to match this guide exactly.
7. **Project » Validate PCB Project** and read the **Messages** panel. Warnings about U1's unused pins are expected. Fix anything else, then check every line of the netlist table.

## 8. Move to the PCB

From the schematic, run **Design » Update PCB Document**. Click **Validate Changes**, then **Execute Changes**, and close the dialog. Your parts appear beside the board, linked by thin lines called the *ratsnest*: the connections you still have to route.

## 9. Set the board shape

An 80 × 70 mm board fits everything, and boards under 100 × 100 mm are usually the cheapest tier at the fabs.

1. Press **2** for 2D view. Set the origin at the bottom-left with **Edit » Origin » Set**.
2. On a mechanical layer, use **Place » Line** to draw a closed 80 × 70 mm rectangle. Exact coordinates can be set in each line's **Properties**.
3. Select all four lines, then **Design » Board Shape » Define Board Shape from Selected Objects**.

## 10. Set the design rules

Open **Design » Rules**. These values are conservative, so any common fab can make them:

| Rule | Value |
|---|---|
| Electrical » Clearance | 0.2 mm |
| Routing » Width (all nets) | min 0.2, preferred 0.3, max 1.5 mm |
| New Width rule, higher priority, for `InNet('+5V') or InNet('GND') or InNet('FAN_NEG')` | preferred 0.8 mm |
| Routing » Routing Via Style | 0.8 mm diameter, 0.4 mm hole |
| Manufacturing » Hole Size | 0.3 to 6.4 mm |

The fan draws its current through those wide nets. 0.8 mm leaves plenty of margin for a small motor.

## 11. Place the parts

Placement decides most of how easy routing will be. Top view, antenna at the top:

```
          antenna hangs over this edge
 ┌─────────────┤▒▒▒ antenna ▒▒▒├─────────────┐
 │ H1          │               │          H2 │
 │ J5 joystick │               │ J1 OLED     │
 │ J4 PIR      │      U1       │ J2 I2C      │
 │             │    ESP32      │             │
 │ C3          │    DevKit     │             │
 │ Q1 R1 R2    │               │ R3          │
 │ D1 C1 C2    │               │ J3 DHT11    │
 │ R4 LED1     └───── USB ─────┘             │
 │ J6 fan        keep this strip clear       │
 │ H3            for the USB plug         H4 │
 └───────────────────────────────────────────┘
```

Why it's laid out this way:

- **The antenna must clear the board.** Copper near the antenna weakens the Wi-Fi the dashboard depends on, so place U1 with pin 1 about 3 mm below the top edge. The antenna then hangs past the board.
- **Keep the USB strip clear.** The plug sits above the board, so tall parts like C1 and J6 must stay out from under it.
- **Keep the fan circuit low on the left, near VIN.** That keeps its current away from the antenna and sensor signals. Q1, D1, C1 and J6 should sit as close together as you can manage.
- **The joystick and PIR go beside their pins,** which all sit together on the left header, so these traces barely need to cross anything.
- **Keep J3 far from U1.** The ESP32 runs warm on Wi-Fi, and that heat would skew the temperature. Better still, use a short cable and hold the sensor away from the board.

**Space** rotates a part while you drag it. Press **3** to see the result in 3D and **2** to return.

## 12. Route the traces

1. **Route » Interactive Routing** (**Ctrl+W**). Click a pad, follow the ratsnest line to its destination, and click to finish.
2. Do the power nets first (`+5V`, `FAN_NEG`, then `+3V3`), then the signals.
3. Route on the top layer where possible. To cross another trace, press **\*** on the number pad while routing: it adds a via and switches to the bottom layer.
4. Leave `GND` for the next step. Copper pours will connect it.

Keep the `FAN_NEG` loop (Q1, D1, J6) short, and keep it away from `JOY_X` and `JOY_Y`, since those analog signals pick up motor noise.

## 13. Pour the ground plane

1. **Place » Polygon Pour** on the **Bottom Layer**. Set **Net: GND**, **Pour Over All Same Net Objects**, and **Remove Dead Copper**. Trace around the whole board.
2. Do the same on the **Top Layer**.
3. **Tools » Polygon Pours » Repour All**.
4. Add a few GND vias (**Place » Via**, set Net to GND) to tie the two pours together, especially near Q1 and C1.

If the antenna doesn't overhang the edge, add **Place » Polygon Pour Cutout** on both layers under it, so there's no copper below the antenna.

## 14. Label the board

Use **Place » String** on **Top Overlay**, 1 mm tall. You'll be glad of these when wiring:

- The pin names at every connector, such as `GND 3V3 SCL SDA`.
- `+` at C1's positive lead and at J6's FAN+. D1 and LED1 need their polarity marks visible.
- The board name and version, like `Smart Station v2.1`.

## 15. Check it before ordering

1. **Tools » Design Rule Check » Run Design Rule Check**. Fix every error. Check that no ratsnest lines remain.
2. Press **3** and look it over in 3D.
3. **Print the board at 1:1** (**File » Print**, scale 1.00). Press your real DevKit and modules onto the paper. This is the cheapest way to catch a wrong footprint, which is the most common first-board mistake.
4. Compare the schematic against section 3 one last time.

## 16. Generate the fabrication files

JLCPCB's guide for Altium Designer 24 to 26 gives these settings. Other fabs accept the same files.

**File » Fabrication Outputs » Gerber Files** (not Gerber X2):

- **General:** units Millimeters, format 0.0001 mm. Tick **Include unconnected mid-layer pads**.
- **Layers:** Top and Bottom Layer, Top and Bottom Overlay, Top and Bottom Solder, Top and Bottom Paste, and the layer with your board outline. Plot only one outline layer.
- **Advanced:** suppress leading zeroes, Plotter Type Unsorted (raster), tick **Optimize change location commands**.

**File » Fabrication Outputs » NC Drill Files:**

- Units Inches, format 2:5, keep leading and trailing zeroes, reference to absolute origin.
- Tick **Optimize change location commands** and **Generate separate NC Drill files for plated & non-plated holes**.

The files land in a `Project Outputs for Smart_Station` folder beside the project. Zip them with no subfolders and upload the zip. Check the fab's preview closely, then order 2 layers, 1.6 mm thick, 1 oz copper, with any finish.

## 17. Bring up the assembled board

Go one step at a time, so a fault is easy to trace:

1. **Bare board:** use a multimeter's continuity mode between `+5V` and `GND`, then `+3V3` and `GND`. Neither should beep.
2. **Solder lowest parts first:** Q1, resistors, diode, ceramic capacitors, LED, headers, then C1 and J6. Check C1's polarity and D1's band.
3. **DevKit only, no modules:** plug it in, check that the antenna end matches the silkscreen, and upload the firmware. Measure 5 V on `+5V` and 3.3 V on `+3V3`.
4. **Fan driver without the motor:** type `1` in the Serial Monitor. LED1 should light. Type `0` and it should go out.
5. **Add one module at a time,** checking the serial log after each: OLED, DHT11, joystick, PIR.
6. **Fan last:** connect the motor and try it. If the board resets when the fan starts, see the brownout entry in the README's troubleshooting table.
