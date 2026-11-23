"""Parametric 3D-printed enclosure for the Smart Station carrier PCB.

Run:  python enclosure.py            (needs: pip install build123d)
Out:  out/base.step|stl, out/lid.step|stl (lid STL is flipped, ready to print),
      out/assembly.step (base + lid + mock PCB/modules for checking in CAD)

Coordinates are the PCB's own (Altium origin at the board's bottom-left corner,
mm): X 0..80, Y 0..70, DevKit antenna towards +Y, USB towards -Y.
The enclosure FRONT (fan, IR window) is +Y; the USB cable leaves the REAR (-Y).
Z = 0 is the desk.

Everything marked VERIFY is a typical dimension for that module, not one taken
from this project's files. Measure your part and edit before printing.
"""

from functools import reduce
from pathlib import Path

from build123d import (
    Box,
    Compound,
    Cylinder,
    Polyline,
    Pos,
    Rectangle,
    RectangleRounded,
    Rot,
    export_step,
    export_stl,
    extrude,
    make_face,
)

# ----- Carrier PCB (from Smart_Station.PcbDoc and the NC drill files) --------
PCB_W, PCB_L, PCB_T = 80.0, 70.0, 1.6
PCB_HOLES = [(4, 4), (76, 4), (4, 66), (76, 66)]  # 3.2 mm, M3
DEVKIT_X = (26.0, 54.0)  # DevKit outline on the board
DEVKIT_Y = (22.44, 74.0)  # overhangs the board's +Y edge by 4 mm
USB_X = 40.0  # DevKit centreline

# ----- DevKit stack-up above the carrier PCB (VERIFY) ------------------------
SOCKET_H = 8.5  # female header the DevKit plugs into
DEVKIT_PIN_SPACER = 2.5  # plastic on the DevKit's own pin strips
DEVKIT_PCB_T = 1.6
DEVKIT_TOP = 15.7  # top of the RF shield above the carrier PCB
USB_CENTRE_ABOVE_PCB = SOCKET_H + DEVKIT_PIN_SPACER + DEVKIT_PCB_T + 1.3
USB_PLUG = (14.0, 10.0)  # opening W x H; must pass the plug's overmould

# ----- Shell ----------------------------------------------------------------
WALL = 2.4
FLOOR = 2.4
LID_T = 3.0
MARGIN_X = 1.0  # PCB edge to inner wall
MARGIN_Y = 7.0  # front/rear; holds the lid posts and the DevKit overhang
STANDOFF_H = 5.0
STANDOFF_D = 7.0
CLEAR_ABOVE_PCB = 45.0  # PCB top to lid underside
CORNER_R = 4.0
POST = 6.5  # lid screw post, square
PILOT_M3 = 2.5  # M3 screw self-tapping into plastic
PILOT_M25 = 2.1
PILOT_M2 = 1.7
LIP_H, LIP_T, LIP_GAP = 3.0, 1.6, 0.25

# ----- Fan motor, 130 size (VERIFY) -----------------------------------------
MOTOR_D = 20.4  # across the round sides
MOTOR_FLAT = 15.4  # across the flats
MOTOR_LEN = 25.2  # can + end cap, without the rear bearing bump
MOTOR_BUSH_D = 6.2  # front bearing boss
MOTOR_PROUD = 3.0  # cradle front sits this far ahead of the front wall
BLADE_D = 75.0  # propeller; only used to check desk clearance
CRADLE_WALL = 2.4

# ----- 1.3 inch OLED, 4-pin I2C (VERIFY) ------------------------------------
OLED_C = (56.0, 17.0)  # module centre on the lid
OLED_PCB = (35.4, 33.5)
OLED_HOLE_PITCH = (30.4, 28.5)
OLED_ACTIVE = (29.4, 14.7)
OLED_ACTIVE_SHIFT = 1.5  # active area centre below PCB centre (away from pins)
OLED_GLASS_T = 1.7  # glass above the module PCB

# ----- KY-023 joystick (VERIFY) ---------------------------------------------
JOY_C = (57.0, 59.0)  # stick centre on the lid; pins point towards -X
JOY_PCB = (34.0, 26.0)
JOY_HOLE_PITCH = (26.67, 20.32)
JOY_STICK_SHIFT = (0.0, 0.0)  # stick centre relative to the hole pattern
JOY_STANDOFF = 13.0  # module PCB top to lid underside
JOY_OPENING_D = 27.0

# ----- HC-SR501 PIR (VERIFY) ------------------------------------------------
PIR_C = (21.0, 17.0)
PIR_PCB = (32.5, 24.5)
PIR_HOLE_PITCH = 28.0  # along the long (X) axis
PIR_DOME_D = 23.4
PIR_STANDOFF = 3.6  # height of the lens' square base

# ----- IR receiver module, receiver standing upright on its PCB (VERIFY) -----
IR_X = 31.5  # window centre on the front wall
IR_PCB_W = 15.6
IR_WINDOW = (10.0, 8.0)

# ----- DHT11: the blue sensor body pokes out through the right wall (VERIFY) -
DHT_Y, DHT_Z_ABOVE_PCB = 20.0, 33.0
DHT_BODY = (15.8, 12.4)  # long side along Y

LDR_C = (12.0, 62.0)
LDR_D = 5.3

OUT = Path(__file__).parent / "out"

# ----- Derived --------------------------------------------------------------
IX0, IX1 = -MARGIN_X, PCB_W + MARGIN_X
IY0, IY1 = -MARGIN_Y, PCB_L + MARGIN_Y
OX0, OX1, OY0, OY1 = IX0 - WALL, IX1 + WALL, IY0 - WALL, IY1 + WALL
CX, CY = (OX0 + OX1) / 2, (OY0 + OY1) / 2
Z_PCB = FLOOR + STANDOFF_H + PCB_T  # PCB top
Z_RIM = Z_PCB + CLEAR_ABOVE_PCB  # top of the base = lid underside
Z_TOP = Z_RIM + LID_T
POSTS = [(IX0, IY0), (IX1 - POST, IY0), (IX0, IY1 - POST), (IX1 - POST, IY1 - POST)]

POCKET_W = MOTOR_FLAT + 0.5
POCKET_R = MOTOR_D / 2 + 0.2
CR_X1 = OX0  # the box wall is the cradle's inner side
CR_X0 = CR_X1 - POCKET_W - CRADLE_WALL
AXIS_X = CR_X1 - POCKET_W / 2
AXIS_Z = Z_RIM - MOTOR_D / 2 - 0.3  # the lid's ear holds the motor down
CR_Y1 = OY1 + MOTOR_PROUD
POCKET_Y1 = CR_Y1 - CRADLE_WALL
POCKET_Y0 = POCKET_Y1 - MOTOR_LEN - 0.4
CR_Y0 = POCKET_Y0 - 2.0
CR_Z0 = AXIS_Z - POCKET_R - CRADLE_WALL
WIRE_Y = CR_Y0 - 5.0


def box(x0, x1, y0, y1, z0, z1):
    (x0, x1), (y0, y1), (z0, z1) = sorted((x0, x1)), sorted((y0, y1)), sorted((z0, z1))
    return Pos((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2) * Box(x1 - x0, y1 - y0, z1 - z0)


def cyl_z(x, y, z0, z1, d):
    return Pos(x, y, (z0 + z1) / 2) * Cylinder(d / 2, z1 - z0)


def cyl_y(x, z, y0, y1, d):
    return Pos(x, (y0 + y1) / 2, z) * Rot(90, 0, 0) * Cylinder(d / 2, y1 - y0)


def cyl_x(y, z, x0, x1, d):
    return Pos((x0 + x1) / 2, y, z) * Rot(0, 90, 0) * Cylinder(d / 2, x1 - x0)


def prism_xz(points, y0, y1):
    face = make_face(Polyline(*[(x, y0, z) for x, z in points], close=True))
    return extrude(face, amount=y1 - y0, dir=(0, 1, 0))


def rounded(w, length, r, z0, z1):
    return Pos(CX, CY, z0) * extrude(RectangleRounded(w, length, r), amount=z1 - z0)


def union(parts):
    return reduce(lambda a, b: a + b, parts)


def grid(centre, pitch):
    cx, cy = centre
    px, py = pitch
    return [(cx + sx * px / 2, cy + sy * py / 2) for sx in (-1, 1) for sy in (-1, 1)]


def build_base():
    shell = rounded(OX1 - OX0, OY1 - OY0, CORNER_R, 0, Z_RIM) - rounded(
        IX1 - IX0, IY1 - IY0, CORNER_R - WALL, FLOOR, Z_RIM
    )
    add = [cyl_z(x, y, FLOOR - 0.1, Z_PCB - PCB_T, STANDOFF_D) for x, y in PCB_HOLES]
    # Lid posts fill the four inner corners, clear of the PCB outline.
    add += [box(x - 0.5 * (x == IX0), x + POST + 0.5 * (x != IX0),
                y - 0.5 * (y == IY0), y + POST + 0.5 * (y != IY0), FLOOR - 0.1, Z_RIM)
            for x, y in POSTS]
    # Motor cradle on the outside of the -X wall, open at the top, with a 45 degree
    # gusset underneath so it prints without support.
    add += [
        box(CR_X0, CR_X1 + 0.1, CR_Y0, CR_Y1, CR_Z0, Z_RIM),
        box(OX0, OX0 + CORNER_R, OY1 - CORNER_R, CR_Y1, CR_Z0, Z_RIM),
        prism_xz([(CR_X0, CR_Z0), (CR_X1 + 0.1, CR_Z0), (CR_X1 + 0.1, CR_Z0 - (CR_X1 - CR_X0))],
                 CR_Y0, CR_Y1),
    ]
    body = shell + union(add)

    usb_z = Z_PCB + USB_CENTRE_ABOVE_PCB
    dht_z = Z_PCB + DHT_Z_ABOVE_PCB
    ir_z1 = Z_RIM - LIP_H - 0.5
    cut = [cyl_z(x, y, 1.0, Z_PCB, PILOT_M3) for x, y in PCB_HOLES]  # 6.4 mm deep
    cut += [cyl_z(x + POST / 2, y + POST / 2, Z_RIM - 14, Z_RIM + 1, PILOT_M3) for x, y in POSTS]
    cut += [
        # motor pocket: round where the motor is round, flat where it is flat
        box(CR_X1 - POCKET_W, CR_X1, POCKET_Y0, POCKET_Y1, AXIS_Z, Z_RIM + 1),
        cyl_y(AXIS_X, AXIS_Z, POCKET_Y0, POCKET_Y1, 2 * POCKET_R)
        & box(CR_X1 - POCKET_W, CR_X1, POCKET_Y0, POCKET_Y1, AXIS_Z - POCKET_R, AXIS_Z + 0.1),
        # front bearing boss and shaft
        box(AXIS_X - MOTOR_BUSH_D / 2 - 0.2, AXIS_X + MOTOR_BUSH_D / 2 + 0.2,
            POCKET_Y1 - 0.1, CR_Y1 + 1, AXIS_Z, Z_RIM + 1),
        cyl_y(AXIS_X, AXIS_Z, POCKET_Y1 - 0.1, CR_Y1 + 1, MOTOR_BUSH_D + 0.4),
        # rear bearing bump and terminals
        box(AXIS_X - 5.5, AXIS_X + 5.5, CR_Y0 - 1, POCKET_Y0 + 0.1, AXIS_Z, Z_RIM + 1),
        cyl_y(AXIS_X, AXIS_Z, CR_Y0 - 1, POCKET_Y0 + 0.1, 11.0),
        # fan wires into the box, towards J6
        cyl_x(WIRE_Y, AXIS_Z, OX0 - 1, IX0 + 1, 5.0),
        # USB plug, rear wall
        box(USB_X - USB_PLUG[0] / 2, USB_X + USB_PLUG[0] / 2, OY0 - 1, IY0 + 1,
            usb_z - USB_PLUG[1] / 2, usb_z + USB_PLUG[1] / 2),
        # IR window, front wall, just under the lid's lip
        box(IR_X - IR_WINDOW[0] / 2, IR_X + IR_WINDOW[0] / 2, IY1 - 1, OY1 + 1,
            ir_z1 - IR_WINDOW[1], ir_z1),
        # DHT11 body, right wall
        box(IX1 - 1, OX1 + 1, DHT_Y - DHT_BODY[0] / 2 - 0.2, DHT_Y + DHT_BODY[0] / 2 + 0.2,
            dht_z - DHT_BODY[1] / 2 - 0.2, dht_z + DHT_BODY[1] / 2 + 0.2),
    ]
    # Sound and air slots in the rear wall behind the buzzer (J8).
    cut += [box(x - 0.9, x + 0.9, OY0 - 1, IY0 + 1, Z_PCB + 3, Z_PCB + 17) for x in (60, 64, 68, 72)]
    return body - union(cut)


def build_lid():
    ear = [
        box(CR_X0, CR_X1 + 0.1, CR_Y0, CR_Y1, Z_RIM, Z_TOP),
        box(OX0, OX0 + CORNER_R, OY1 - CORNER_R, CR_Y1, Z_RIM, Z_TOP),
    ]
    lip_w, lip_l = IX1 - IX0 - 2 * LIP_GAP, IY1 - IY0 - 2 * LIP_GAP
    lip = rounded(lip_w, lip_l, CORNER_R - WALL - LIP_GAP, Z_RIM - LIP_H, Z_RIM + 0.1) - Pos(
        CX, CY, Z_RIM - LIP_H - 1
    ) * extrude(Rectangle(lip_w - 2 * LIP_T, lip_l - 2 * LIP_T), amount=LIP_H + 2)
    lip -= union([box(x - 0.8, x + POST + 0.8, y - 0.8, y + POST + 0.8, Z_RIM - LIP_H - 1, Z_RIM)
                  for x, y in POSTS])

    oled_holes = grid(OLED_C, OLED_HOLE_PITCH)
    joy_holes = grid(JOY_C, JOY_HOLE_PITCH)
    joy_holes = [(x - JOY_STICK_SHIFT[0], y - JOY_STICK_SHIFT[1]) for x, y in joy_holes]
    pir_holes = [(PIR_C[0] - PIR_HOLE_PITCH / 2, PIR_C[1]), (PIR_C[0] + PIR_HOLE_PITCH / 2, PIR_C[1])]
    oled_h, ir_fence_y0 = OLED_GLASS_T + 0.2, IY1 - LIP_GAP - LIP_T - 21

    add = ear + [lip]
    add += [cyl_z(x, y, Z_RIM - oled_h, Z_RIM + 0.1, 4.5) for x, y in oled_holes]
    add += [cyl_z(x, y, Z_RIM - JOY_STANDOFF, Z_RIM + 0.1, 6.0) for x, y in joy_holes]
    add += [cyl_z(x, y, Z_RIM - PIR_STANDOFF, Z_RIM + 0.1, 4.8) for x, y in pir_holes]
    # Two fences locate the IR module, which sticks to the lid with foam tape.
    add += [box(IR_X + s * (IR_PCB_W / 2 + 0.2), IR_X + s * (IR_PCB_W / 2 + 1.4),
                ir_fence_y0, IY1 - LIP_GAP - LIP_T + 0.1, Z_RIM - 3, Z_RIM + 0.1) for s in (-1, 1)]
    body = rounded(OX1 - OX0, OY1 - OY0, CORNER_R, Z_RIM, Z_TOP) + union(add)

    # The OLED's top edge (pins) faces the rear, so it reads upright from the front.
    win_cy = OLED_C[1] + OLED_ACTIVE_SHIFT
    win_w, win_l = OLED_ACTIVE[0] + 2.0, OLED_ACTIVE[1] + 2.0
    cut = [
        box(OLED_C[0] - win_w / 2, OLED_C[0] + win_w / 2, win_cy - win_l / 2, win_cy + win_l / 2,
            Z_RIM - 1, Z_TOP + 1),
        box(OLED_C[0] - win_w / 2 - 1.5, OLED_C[0] + win_w / 2 + 1.5,
            win_cy - win_l / 2 - 1.5, win_cy + win_l / 2 + 1.5, Z_TOP - 1.5, Z_TOP + 1),
        cyl_z(JOY_C[0], JOY_C[1], Z_RIM - 1, Z_TOP + 1, JOY_OPENING_D),
        cyl_z(PIR_C[0], PIR_C[1], Z_RIM - 1, Z_TOP + 1, PIR_DOME_D),
        cyl_z(LDR_C[0], LDR_C[1], Z_RIM - 1, Z_TOP + 1, LDR_D),
    ]
    cut += [cyl_z(x, y, Z_RIM - oled_h - 1, Z_TOP - 1.0, PILOT_M25) for x, y in oled_holes]
    cut += [cyl_z(x, y, Z_RIM - JOY_STANDOFF - 1, Z_RIM - JOY_STANDOFF + 8, PILOT_M25) for x, y in joy_holes]
    cut += [cyl_z(x, y, Z_RIM - PIR_STANDOFF - 1, Z_TOP - 1.0, PILOT_M2) for x, y in pir_holes]
    for x, y in POSTS:
        cut.append(cyl_z(x + POST / 2, y + POST / 2, Z_RIM - 1, Z_TOP + 1, 3.4))
        cut.append(cyl_z(x + POST / 2, y + POST / 2, Z_TOP - 1.6, Z_TOP + 1, 6.4))
    return body - union(cut)


def build_mocks():
    """Keep-out volumes for the PCB, its tall parts, jumper plugs and the modules."""
    dupont = 21.5  # top of a jumper plug on a pin header, above the PCB
    m = {
        "pcb": box(0, PCB_W, 0, PCB_L, Z_PCB - PCB_T, Z_PCB),
        "devkit": box(*DEVKIT_X, *DEVKIT_Y, Z_PCB + SOCKET_H, Z_PCB + DEVKIT_TOP)
        + box(27.3, 29.8, 31, 69.5, Z_PCB, Z_PCB + SOCKET_H)
        + box(50.2, 52.7, 31, 69.5, Z_PCB, Z_PCB + SOCKET_H),
        "usb_plug": box(USB_X - 5.75, USB_X + 5.75, OY0 - 15, DEVKIT_Y[0],
                        Z_PCB + USB_CENTRE_ABOVE_PCB - 4, Z_PCB + USB_CENTRE_ABOVE_PCB + 4),
        "c1": cyl_z(6, 11, Z_PCB, Z_PCB + 11.5, 8),
        "j6": box(0, 10, 22, 32, Z_PCB, Z_PCB + 10),
        "buzzer": cyl_z(66.73, 12, Z_PCB, Z_PCB + 12, 12),
        "plugs": union([box(x0, x1, y0, y1, Z_PCB, Z_PCB + dupont) for x0, x1, y0, y1 in [
            (59.9, 70.1, 60.7, 63.3), (60.6, 70.8, 44.7, 47.3), (67.7, 70.3, 23.2, 30.8),
            (9.7, 12.3, 34.6, 42.3), (14.7, 17.3, 53.1, 65.8), (74.7, 77.3, 55.4, 60.6),
            (16.7, 19.3, 34.6, 42.3)]]),
    }
    ox, oy = OLED_C
    oz = Z_RIM - OLED_GLASS_T - 0.2
    m["oled"] = (
        box(ox - OLED_PCB[0] / 2, ox + OLED_PCB[0] / 2, oy - OLED_PCB[1] / 2, oy + OLED_PCB[1] / 2,
            oz - PCB_T - 2, oz)
        + box(ox - 5.1, ox + 5.1, oy - OLED_PCB[1] / 2 + 0.3, oy - OLED_PCB[1] / 2 + 2.9, oz - 23, oz)
    )
    jx, jy = JOY_C[0] - JOY_STICK_SHIFT[0], JOY_C[1] - JOY_STICK_SHIFT[1]
    jz = Z_RIM - JOY_STANDOFF
    m["joystick"] = (
        box(jx - JOY_PCB[0] / 2, jx + JOY_PCB[0] / 2, jy - JOY_PCB[1] / 2, jy + JOY_PCB[1] / 2,
            jz - PCB_T - 2.5, jz)
        + box(JOY_C[0] - 8, JOY_C[0] + 8, JOY_C[1] - 8, JOY_C[1] + 8, jz, Z_RIM - 0.5)
        + box(jx - JOY_PCB[0] / 2 - 20, jx - JOY_PCB[0] / 2, jy - 6.4, jy + 6.4, jz - 4.2, jz + 1)
    )
    px, py = PIR_C
    pz = Z_RIM - PIR_STANDOFF
    m["pir"] = (
        box(px - PIR_PCB[0] / 2, px + PIR_PCB[0] / 2, py - PIR_PCB[1] / 2, py + PIR_PCB[1] / 2,
            pz - PCB_T - 11, pz)
        + box(px - 3.9, px + 3.9, py - PIR_PCB[1] / 2 + 0.5, py - PIR_PCB[1] / 2 + 3.1, pz - 23, pz)
        + cyl_z(px, py, Z_RIM - 1, Z_TOP + 9, 23.0)
    )
    iy1 = IY1 - LIP_GAP - LIP_T - 0.3
    m["ir"] = box(IR_X - IR_PCB_W / 2, IR_X + IR_PCB_W / 2, iy1 - 20, iy1, Z_RIM - 2.6, Z_RIM - 1.0) + box(
        IR_X - 3, IR_X + 3, iy1 - 5.5, iy1, Z_RIM - 10.1, Z_RIM - 2.6
    ) + box(IR_X - 4, IR_X + 4, iy1 - 40, iy1 - 20, Z_RIM - 5.2, Z_RIM - 1.0)
    dz = Z_PCB + DHT_Z_ABOVE_PCB
    m["dht11"] = box(IX1 - 1.6, IX1, DHT_Y - 9, DHT_Y + 19, dz - 7, dz + 7) + box(
        IX1 - 0.1, IX1 + 5.5 - 1.6, DHT_Y - DHT_BODY[0] / 2, DHT_Y + DHT_BODY[0] / 2,
        dz - DHT_BODY[1] / 2, dz + DHT_BODY[1] / 2
    ) + box(IX1 - 4.2, IX1 - 1.6, DHT_Y + 19, DHT_Y + 39, dz - 4, dz + 4)
    m["motor"] = (
        cyl_y(AXIS_X, AXIS_Z, POCKET_Y0 + 0.2, POCKET_Y1 - 0.2, MOTOR_D)
        & box(AXIS_X - MOTOR_FLAT / 2, AXIS_X + MOTOR_FLAT / 2, POCKET_Y0, POCKET_Y1, 0, 100)
    ) + cyl_y(AXIS_X, AXIS_Z, POCKET_Y1 - 0.3, CR_Y1 + 8, 2.0)
    return m


def check(base, lid, mocks):
    """Report every overlap between the printed parts and the keep-out volumes."""
    solids = {"base": base, "lid": lid, **mocks}
    names = list(solids)
    clashes = []
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            if a == "base" and b == "lid":
                continue
            common = solids[a] & solids[b]
            vol = common.volume if common is not None else 0.0
            if vol > 0.05:
                clashes.append((a, b, vol))
    return clashes


def main():
    OUT.mkdir(exist_ok=True)
    base, lid, mocks = build_base(), build_lid(), build_mocks()

    print(f"outer size  {OX1 - CR_X0:.1f} x {CR_Y1 - OY0:.1f} x {Z_TOP:.1f} mm "
          f"(box alone {OX1 - OX0:.1f} x {OY1 - OY0:.1f})")
    print(f"motor axis  z = {AXIS_Z:.1f} mm, {BLADE_D:.0f} mm blade clears the desk by "
          f"{AXIS_Z - BLADE_D / 2:.1f} mm")
    print(f"USB window  centre z = {Z_PCB + USB_CENTRE_ABOVE_PCB:.1f} mm")
    for name, part in (("base", base), ("lid", lid)):
        print(f"{name:5s} valid={part.is_valid} solids={len(part.solids())} volume={part.volume / 1000:.1f} cm3")

    clashes = check(base, lid, mocks)
    for a, b, vol in clashes:
        print(f"CLASH {a} / {b}: {vol:.1f} mm3")
    print("interference check:", "FAILED" if clashes else "no overlaps")

    export_step(base, str(OUT / "base.step"))
    export_step(lid, str(OUT / "lid.step"))
    export_stl(base, str(OUT / "base.stl"))
    export_stl(Pos(0, 0, Z_TOP) * Rot(180, 0, 0) * lid, str(OUT / "lid.stl"))
    export_step(Compound(children=[base, lid, *mocks.values()]), str(OUT / "assembly.step"))
    return 1 if clashes else 0


if __name__ == "__main__":
    raise SystemExit(main())
