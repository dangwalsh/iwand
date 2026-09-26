# iWand Electrical

KiCad schematic for the iWand color-identifying wand (Seeed XIAO ESP32S3 + GY-33 color sensor + DFPlayer Mini + AO3400 power gating). See `firmware/CLAUDE.md` for how the pin assignments here map to the firmware.

## Dependencies

- **KiCad 7.0 or newer** (developed with KiCad 10). The schematic (`iwand.kicad_sch`) is in the KiCad 7 s-expression format (`version 20230121`). Older KiCad versions won't open it.

The module symbols (`wand_parts:XIAO_ESP32S3`, `wand_parts:DFPlayer_Mini`) are embedded directly in `iwand.kicad_sch`, so there's no external symbol library to install. ERC warns that the `wand_parts` library isn't configured and that `Device:Q_NMOS_GDS` is missing from KiCad's current `Device` library. Both warnings are harmless, because the embedded copies are used.

## Footprints

Project footprints live in `iwand.pretty`, registered through the project's `fp-lib-table` as `iwand`:

| Footprint | Used by | Source |
|---|---|---|
| `XIAO_ESP32S3_Socket` | U1 | Pad positions from Seeed's `XIAO-ESP32-S3-DIP`, without its castellated SMD pads. USB-C is at +X; B+/B- on the fab layer mark the underside battery pads. |
| `DFPlayer_Mini_Socket` | U3 | DFR0299 datasheet drawing: 20.32 mm square, 2 x 8 pins, rows 18.034 mm apart. |
| `AO3400_SOT23_Adapter` | Q1, Q2 | 1 x 3 pins in the adapter board's S-D-G order (adapter labels 2-3-1). Pads are numbered for `Q_NMOS_GDS`. The adapter outline (~7.6 x 10 mm) is estimated from a photo. |

Everything else is stock KiCad:
- DIN0207 through-hole resistors, lying flat
- a 3 mm disc capacitor
- a C&K OS102011MA1QN1-style right-angle slide switch
- JST-PH side-entry sockets (J1-J4)
- a 1 x 2 pin header (J5)

## Board

The board is 75.9 x 22 mm, laid out as a "spine" along the wand. From the rear:
- J4 battery, J2 speaker and J3 button plugs, facing the rear
- the XIAO, with USB-C over the lower long edge; R1-R6 lie flat underneath it
- Q1/Q2 with J5, SW2 and C1
- the DFPlayer
- J1, facing forward to the GY-33

In the low-profile (~5 mm) sockets, the XIAO sits about 7.5 mm up. Its courtyard therefore covers only its socket strips and USB plug, so parts up to about 5 mm tall can go underneath. The socket 3D models are KiCad's 8.5 mm models scaled down to 5 mm.

## Mounting and off-board parts

- U1 and U3 plug into female header sockets so they stay removable. Q1/Q2 are AO3400s already soldered to SOT-23 adapter boards, which are mounted flat on their pins.
- The GY-33, speaker, wake button and battery sit off the board on JST-PH connectors:
  - **J1 (GY-33):** a 4-wire pigtail soldered into the sensor's GND/DR/CT/VCC holes, in that order.
  - **J2:** speaker.
  - **J3:** wake button.
  - **J4:** LiPo.
- JST battery polarity isn't standardised. Check each battery against the board's + mark before plugging it in.
- The XIAO's battery pads are on its underside, not on its headers. **J5** takes two short wires from those BAT+/BAT- pads so the XIAO can run from, and charge, the battery on J4.

## Power and charging

- The battery stays in the wand and charges through the XIAO's USB-C (about 100 mA).
- **SW2** is the power switch, in the battery + lead (J4 → SW2 → VBAT). Off disconnects everything, including the charger, so the battery only charges with SW2 on.
- The XIAO's charge LED sits next to its USB-C port, so the enclosure needs a window or light pipe there.
- **R5/R6** (1M each) divide VBAT by 2 into D10, which is an ADC input. **C1** (100 nF) steadies the reading. The divider draws about 2 µA, and nothing when SW2 is off.
