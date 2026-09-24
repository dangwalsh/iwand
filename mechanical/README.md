# iWand — Mechanical

iWand is a hand-held device that identifies the color of an object it's pointed at. This directory holds the mechanical/enclosure design; sibling directories at the repo root hold the corresponding work for the same product — `firmware/` (embedded code) and `electrical/` (KiCad schematic, `iwand.kicad_pro`/`.kicad_sch`, title block "iWand").

This directory is currently empty.

## Dependencies

No CAD tool has been chosen yet, so there's nothing to install for this directory yet. Add this section once a mechanical design tool (and any exported file formats other collaborators will need) is decided.

## Components to accommodate

Per the electrical schematic (source of truth: `electrical/iwand.kicad_sch`):

- **U1** — Seeed XIAO ESP32S3 (MCU)
- **U2** — GY-33 color sensor breakout — needs an aperture/alignment at the "pointing" end of the wand so it has a clear view of the target object
- **U3** — DFPlayer Mini (audio playback)
- **LS1** — 8Ω speaker — needs an acoustic port/opening
- **BT1** — 3.7V LiPo cell — needs a compartment, and physical access if it's user-replaceable/rechargeable
- **SW1** — momentary pushbutton — needs to be reachable/actuable from outside the enclosure as the wand's trigger
- **Q1/Q2, R1–R4** — small SMD MOSFETs/resistors on the PCB, no direct enclosure implications

The schematic notes that U1, U2, and U3 must be mounted via female header sockets rather than soldered directly, since all three need to be removable/replaceable — worth keeping in mind for board stack height and connector clearance. U2's exact module/footprint is still TBD on the electrical side, so its physical footprint isn't finalized either.
