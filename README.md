# iWand

iWand is a hand-held device that identifies the color of an object it's pointed at. Point it at something, press the button, and it announces the color out loud.

## Repo layout

- [`firmware/`](firmware/README.md) — embedded code for the Seeed XIAO ESP32S3, using a GY-33 color sensor and a DFPlayer Mini + speaker for audio announcement
- [`electrical/`](electrical/README.md) — KiCad schematic (`iwand.kicad_pro`/`.kicad_sch`)
- [`mechanical/`](mechanical/README.md) — enclosure design (no CAD files yet)

Each directory's README has setup/dependency details specific to that part of the project; `firmware/CLAUDE.md` has the full architecture writeup (wake/sleep flow, pin assignments, library APIs).

## Status

Early stage: firmware is a single-file sketch, the schematic has no footprints assigned yet, and mechanical design hasn't started.
