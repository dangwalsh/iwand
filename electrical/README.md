# iWand Electrical

KiCad schematic for the iWand color-identifying wand (Seeed XIAO ESP32S3 + GY-33 color sensor + DFPlayer Mini + AO3400 power gating). See `firmware/CLAUDE.md` for how the pin assignments here map to the firmware.

## Dependencies

- **KiCad 7.0 or newer** — schematic file (`iwand.kicad_sch`) is in the KiCad 7 s-expression format (`version 20230121`). Older KiCad versions won't open it.

All part symbols (`wand_parts:XIAO_ESP32S3`, `wand_parts:GY33_Breakout`, `wand_parts:DFPlayer_Mini`, plus standard `Device:`/`Switch:` library parts) are embedded directly in `iwand.kicad_sch` — there's no external symbol library to install.

No footprints are assigned yet, so there's nothing to route/fab from this schematic as-is.
