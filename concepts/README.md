# Concepts

Other ways to build iWand, explored next to the product. Nothing in this folder is part of the firmware, PCB or enclosure build.

iWand helps colour-blind people identify the true colour of an object. Users can see and judge brightness but can't tell hues apart. So none of these designs may show a result by colour alone.

| Concept | Measures light with | Decides the name with | Tells the user by | Status |
|---|---|---|---|---|
| Product (repo root) | GY-33 sensor module | ESP32-S3 firmware | Spoken name | Built and tested |
| [Analog](analog/README.md) | 3 photodiodes + colour filters | Comparator network, no MCU | Labelled LED position | Front end sized; parts being sourced for a breadboard test |
| [Mechanical](mechanical/README.md) | The user's eye, matching brightness | Order of three dials + lookup card | Printed name on the card | Concept and illustrated page |

## Shared colour thresholds

All three use the firmware's colour thresholds (`classifyColour()` in `firmware/firmware.ino`). Below is the table they were derived from. Hue is in degrees; saturation and brightness are gamma-encoded (2.2) and run 0–1 between the black and white points. Each range includes its lower limit and excludes its upper limit.

| Colour | Hue (°) | Saturation | Brightness |
|---|---|---|---|
| Black | any | any | below 0.25 |
| White | any | below 0.15 | 0.80 and up |
| Gray | any | below 0.15 | 0.25–0.80 |
| Brown | 10–50 | 0.15 and up | 0.25–0.70 |
| Red | 330–360 or 0–10 | 0.15 and up | 0.25 and up |
| Red | 10–15 | 0.15 and up | 0.70 and up |
| Orange | 15–40 | 0.15 and up | 0.70 and up |
| Yellow | 40–50 | 0.15 and up | 0.70 and up |
| Yellow | 50–70 | 0.15 and up | 0.25 and up |
| Green | 70–150 | 0.15 and up | 0.25 and up |
| Cyan | 150–200 | 0.15 and up | 0.25 and up |
| Blue | 200–230 | 0.15 and up | 0.25 and up |
| Purple | 230–330 | 0.15 and up | 0.25 and up |

When the firmware's thresholds change, check whether the derived values in the concept READMEs need to change too.

### Converting to linear light

Photodiodes, and the mechanical version's attenuator dials, work in linear light, but the firmware applies gamma 2.2 before it compares anything. Converted to linear:

| Firmware threshold | Linear fraction of white | Optical density (D) |
|---|---|---|
| Black: brightness below 0.25 | below 0.047 | above 1.33 |
| Dark band (Brown): brightness below 0.70 | below 0.456 | above 0.34 |
| White: brightness 0.80 and up | 0.612 and up | below 0.21 |
| No hue: saturation below 0.15 | dimmest channel above 0.93 × brightest | channels within 0.03 |

The hue sector (which channel is brightest and which is dimmest) is the same in linear and gamma-encoded terms, because gamma never changes which value is larger. The boundaries inside a sector (10°, 15°, 40°…) are not: in linear terms they move with saturation, so they have to be tuned on real samples.
