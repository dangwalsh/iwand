# iWand — Mechanical

iWand is a hand-held device that identifies the color of an object it's pointed at. This directory holds the mechanical/enclosure design; sibling directories at the repo root hold the corresponding work for the same product — `firmware/` (embedded code) and `electrical/` (KiCad schematic, `iwand.kicad_pro`/`.kicad_sch`, title block "iWand").

This directory is currently empty.

## Dependencies

No CAD tool has been chosen yet, so there's nothing to install for this directory yet. Add this section once a mechanical design tool (and any exported file formats other collaborators will need) is decided.

## Components to accommodate

Per the electrical schematic (source of truth: `electrical/iwand.kicad_sch`):

On the main PCB:

- **U1:** Seeed XIAO ESP32S3 (MCU), 21 x 17.8 mm, in female header sockets. Its USB-C port must stay reachable for charging and flashing.
- **U3:** DFPlayer Mini (audio playback), 20.3 x 20.3 mm, in female header sockets. Its microSD slot needs clearance.
- **Q1/Q2:** AO3400 MOSFETs on SOT-23 adapter boards (~7.6 x 10 mm), mounted flat on their pins. **R1–R4:** through-hole resistors, mounted upright.
- **J1–J4:** JST-PH side-entry sockets, which need room for the plugs and cable bends.

Off-board, on JST-PH cables:

- **GY-33 color sensor (J1):** 24.3 x 26.7 mm, with four corner screw holes about 21 x 19.7 mm apart (measured from a photo, to be confirmed with calipers). The sensor and LEDs face forward, so it needs an aperture at the pointing end of the wand. Allow about 20 mm behind it for the cable.
- **Speaker (J2):** 8 Ω, 28 mm round, ~5 mm deep. It needs a grille, ideally with a closed back volume.
- **Wake button SW1 (J3):** a panel-mount momentary pushbutton, reachable as the wand's trigger.
- **LiPo (J4):** 3.7 V, with a compartment and access for unplugging it.

The widest parts are the speaker (28 mm) and the GY-33's face, which together set the enclosure's minimum cross-section.
