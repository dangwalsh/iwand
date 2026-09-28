# iWand

iWand is a hand-held device that identifies the color of an object it's pointed at. Point it at something, press the button, and it announces the color out loud.

## Repo layout

- [`firmware/`](firmware/README.md) — embedded code for the Seeed XIAO ESP32S3, using a GY-33 color sensor and a DFPlayer Mini + speaker for audio announcement
- [`electrical/`](electrical/README.md) — KiCad project: schematic, routed 75.9 × 22 mm two-layer PCB, project footprint library (`iwand.pretty`), 3D models, and ready-to-order Gerbers (`fab/iwand-gerbers.zip`)
- [`mechanical/`](mechanical/README.md) — enclosure design in Onshape, with a STEP model of the populated PCB (`iwand-pcb.step`) exported from KiCad

Each directory's README has setup/dependency details specific to that part of the project; `firmware/CLAUDE.md` has the full architecture writeup (wake/sleep flow, pin assignments, library APIs).

## Status

- **Firmware:** a single-file sketch. It runs on hardware with calibration and spoken prompts. The battery-voltage monitor (D10) isn't read yet.
- **Electrical:** the PCB is routed and passes KiCad's design rules and schematic parity checks, with Gerbers ready to order. It hasn't been built or tested yet.
- **Mechanical:** enclosure design in Onshape has just started, from the exported PCB model. The layout along the wand, the component sizes and the required openings are in `mechanical/README.md`.
