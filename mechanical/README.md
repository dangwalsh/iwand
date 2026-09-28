# iWand — Mechanical

iWand is a hand-held device that identifies the color of an object it's pointed at. This directory holds the mechanical/enclosure design; sibling directories at the repo root hold the corresponding work for the same product — `firmware/` (embedded code) and `electrical/` (KiCad schematic, `iwand.kicad_pro`/`.kicad_sch`, title block "iWand").

The enclosure is designed in Onshape. This directory holds `iwand-pcb.step`, a STEP model of the populated PCB exported from KiCad, which is the reference the enclosure is built around.

## Dependencies

- **Onshape** (browser-based) for the enclosure design.
- **KiCad 10** (`kicad-cli`) to re-export `iwand-pcb.step` after the board changes.

## PCB model

`iwand-pcb.step` is the board plus every part that has a 3D model, in millimetres. To re-export it after changing `electrical/iwand.kicad_pcb`, run this from the repo root:

```
"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe" pcb export step --subst-models --force -o mechanical/iwand-pcb.step electrical/iwand.kicad_pcb
```

The XIAO and DFPlayer models come from `electrical/3dmodels/`, and the rest come from KiCad's standard library.

To use it in Onshape, import the file (Import, or drag it onto the Documents page), then insert the resulting Part Studio into an Assembly with the enclosure. After a re-export, use "Update" on the imported part to pick up the new version.

The model leaves out:

- **Q1/Q2:** the `AO3400_SOT23_Adapter` footprint has no 3D model. Allow about 7.6 x 10 mm lying flat on their pins.
- **All off-board parts:** the GY-33, speaker, SW1, LiPo and the JST plugs and cables. Model these in Onshape from the sizes below.

## Components to accommodate

Per the electrical schematic (source of truth: `electrical/iwand.kicad_sch`):

On the main PCB:

- **U1:** Seeed XIAO ESP32S3 (MCU), 21 x 17.8 mm, in female header sockets. Its USB-C port must stay reachable for charging and flashing.
- **U3:** DFPlayer Mini (audio playback), 20.3 x 20.3 mm, in female header sockets. Its microSD slot opens toward the front of the wand, right beside J1, so there's no finger room to swap the card in place. Pull the DFPlayer out of its sockets to change the audio files.
- **Q1/Q2:** AO3400 MOSFETs on SOT-23 adapter boards (~7.6 x 10 mm), mounted flat on their pins. **R1–R4:** through-hole resistors, mounted upright.
- **J1–J4:** JST-PH side-entry sockets, which need room for the plugs and cable bends.
- **SW2:** a right-angle slide switch (power). Its lever sticks out about 4 mm past the same long edge as the USB-C, about 13 mm forward of it, so one side opening or two small cut-outs can serve both.

The board is 75.9 x 22 mm. Its tallest point is the top of the XIAO, about 10–11 mm above the board (3.8 mm low-profile socket + 2.5 mm pin spacer + the module). Pin tails stick out up to about 3 mm below. The XIAO's USB-C port sits over one long edge of the board and must line up with a slot in the enclosure wall.

**Charging:** the battery stays in and charges over USB-C, but only with SW2 switched on. The XIAO's small charge LED sits on top of the module, right beside the USB-C port. Leave a window or light pipe above it, or make the USB slot wide enough to show the glow.

Off-board, on JST-PH cables:

- **GY-33 color sensor (J1):** 24.3 x 26.7 mm, with four corner screw holes about 21 x 19.7 mm apart (measured from a photo, to be confirmed with calipers). The sensor and LEDs face forward, so it needs an aperture at the pointing end of the wand. Allow about 20 mm behind it for the cable.
- **Speaker (J2):** 8 Ω, 28 mm round, ~5 mm deep. It needs a grille, ideally with a closed back volume.
- **Wake button SW1 (J3):** a panel-mount momentary pushbutton, reachable as the wand's trigger.
- **LiPo (J4):** 3.7 V, with a compartment. It stays connected and charges in place, so it only needs access for replacement.

The widest parts are the speaker (28 mm) and the GY-33's face, which together set the enclosure's minimum cross-section.

## Layout along the wand

From rear to front: **speaker (rear end cap) → battery → PCB → GY-33 (tip)**.

- The battery, speaker and button plugs (J4, J2, J3) face the rear, toward the battery and speaker, which keeps their leads short. The plug housings stick out about 7 mm behind the board, so leave that gap, plus room for the cables to bend, before the battery.
- The battery sits on the wand's centre line, so the weight stays centred. Any cell that fits the tube's inside diameter will do. It doesn't need to fit under the board.
- J1 faces forward. The GY-33 cable runs from it to the sensor at the tip, and needs about 20 mm behind the sensor.
