# Smart Station enclosure

A two-part printed box for the carrier PCB: a base that holds the board and the fan motor, and a lid that carries the OLED, joystick, PIR, IR receiver and light sensor. Outer size is 105 × 92 × 57 mm including the fan cradle.

The front is the DevKit's antenna side. The fan and IR receiver face you, and the USB cable leaves the rear.

## Measure before printing

The PCB geometry comes from `Smart_Station.PcbDoc`. The module dimensions do not: they are typical values for each part. Check each constant marked `VERIFY` in `enclosure.py` against your part with calipers, edit it, and regenerate.

| Constant | What to measure |
|---|---|
| `SOCKET_H`, `DEVKIT_PIN_SPACER` | Height of the DevKit above the carrier board. Sets the USB window height |
| `USB_PLUG` | Width and thickness of your USB plug's body. The whole plug passes through the wall |
| `OLED_HOLE_PITCH`, `OLED_ACTIVE_SHIFT` | Hole spacing, and how far the lit area sits below the board's centre |
| `JOY_HOLE_PITCH`, `JOY_STICK_SHIFT` | Hole spacing, and whether the stick is centred between the holes |
| `PIR_HOLE_PITCH`, `PIR_STANDOFF` | Hole spacing, and the height of the lens' square base |
| `IR_PCB_W` | Width of the IR module's board |
| `DHT_BODY` | The blue sensor body, long side then short side |
| `MOTOR_D`, `MOTOR_FLAT`, `MOTOR_LEN`, `BLADE_D` | Motor across the round sides, across the flats, body length, and propeller diameter |

## Regenerate

```bash
pip install build123d
```

```bash
python enclosure.py
```

The script writes `out/base.stl`, `out/lid.stl`, STEP versions of both, and `out/assembly.step` with mock-ups of the PCB and modules for checking the fit in CAD. It also tests every part against those mock-ups and exits with an error if anything overlaps.

## Print

- `base.stl` prints as modelled, floor on the bed. `lid.stl` is already flipped so its outer face is on the bed.
- No supports. The motor cradle has a 45° gusset, and the windows bridge at most 16 mm.
- 0.2 mm layers, 3 perimeters, 15 to 20 % infill. The screws tap into the perimeters, so don't use fewer.
- PLA is fine indoors. PETG is tougher around the screw posts.

## Screws

All screws cut their own thread in the plastic. Drive them in slowly the first time.

| Where | Screw | Count |
|---|---|---|
| PCB to standoffs | M3 × 6 | 4 |
| Lid to posts | M3 × 10 | 4 |
| Joystick | M2.5 × 8 | 4 |
| OLED | M2.5 × 5, no longer | 4 |
| PIR | M2 × 6 | 2 |

## Assemble

1. **Lid, lying face down.** Screw on the OLED with its pins towards the rear edge, the joystick with its pins towards the PIR side, and the PIR with its dome through the hole.
2. Stick the IR module between the two fences with foam tape, component side up, the receiver against the front lip.
3. Push the photoresistor into its hole from inside and fix it with a drop of hot glue.
4. **Base.** Feed the motor's wires through the hole in the left wall, drop the motor into its cradle with the shaft to the front, and screw the PCB down.
5. Plug in the DevKit, then push the DHT11's body out through the right-wall window from inside, pins towards the front. It goes in after the PCB because it overhangs the board's edge.
6. Wire the motor to J6 and the modules to their headers, using the pin tables in the main README.
7. Fit the lid, then push the USB plug through the rear window into the DevKit.

Fit the propeller last, and check that it spins clear of the desk and the front wall before powering the fan.

## Known limits

- The PIR points straight up. It sees someone leaning over the desk better than someone walking past.
- The DHT11 and IR module are held by a friction fit and tape, not screws.
- The motor is held down by the lid, so it is loose whenever the lid is off.
