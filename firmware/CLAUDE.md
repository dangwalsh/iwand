# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

iWand is a hand-held device that identifies the color of an object it's pointed at. This `firmware/` directory holds the embedded code that runs on the device; sibling directories at the repo root (`electrical/`, `mechanical/`) hold the corresponding hardware design work for the same product. `electrical/` has a KiCad project (`iwand.kicad_pro`/`.kicad_sch`) that is the source of truth for pin assignments below; `mechanical/` has a `README.md` with enclosure-relevant context but no design files yet.

## Hardware

- MCU: ESP32-S3
- Sensor: GY-33 color sensor, connected to the ESP32-S3 over I2C
- Audio: 8Ω speaker driven by a DFRobot DFPlayer Mini (UART)
- Power gating: AO3400 N-channel MOSFETs cut power to the GY-33 and the DFPlayer Mini independently when each is not in use, to conserve battery
- Input: a momentary switch wakes the ESP32-S3 from deep sleep

## Architecture

There is no `loop()` logic. The device spends nearly all its time in ESP32-S3 deep sleep; a button press on `WAKE_BUTTON_PIN` triggers an `ext0` wakeup, which resets the MCU and re-enters `setup()` (deep sleep wake is a reset, not a resume — this is why `loop()` is intentionally empty). `setup()` runs the full cycle each time:

1. Power the GY-33 via its MOSFET, take one reading, power it back down.
2. Map the detected colour name to a track number and power the DFPlayer Mini via its MOSFET to play the matching audio file, power it back down once playback finishes (or times out).
3. Re-arm `ext0` wakeup on the button pin and re-enter deep sleep.

**Board:** Seeed XIAO ESP32S3. GPIO pin assignments come from the schematic pinout (silkscreen D-labels translated to GPIO numbers per Seeed's pinout table):

| Silkscreen | GPIO | Purpose |
|---|---|---|
| D0 | GPIO1 | R1 → Q1 (AO3400) gate — GY-33 power switch |
| D1 | GPIO2 | R3 → Q2 (AO3400) gate — DFPlayer power switch |
| D3 | GPIO4 | Momentary switch to GND — wake from deep sleep (ext0) |
| D4 | GPIO5 | I2C SDA to GY-33/TCS34725 |
| D5 | GPIO6 | I2C SCL to GY-33/TCS34725 |
| D8 | GPIO7 | UART RX from DFPlayer TXD |
| D9 | GPIO8 | UART TX to DFPlayer RXD |

Pins were deliberately grouped: D0/D1 are the two MOSFET gates, D3 is the wake button, D4/D5 are I2C, D8/D9 are UART.

Note: the sensor MOSFET gate was moved from the schematic's original D2 (GPIO3) to D0 (GPIO1), since GPIO3 is one of the ESP32-S3's four strapping pins (0, 3, 45, 46), sampled at boot/reset to configure chip behavior. D0/GPIO1 has no competing native function. The schematic's R1 net and the `XIAO_ESP32S3` symbol's pin 4 have both been updated from `D2` to `D0` to match. The wake button and DFPlayer-MOSFET-gate nets were later swapped between D1 and D3 (schematic net labels `D1`/`D3_BTN`) purely to group pins more logically; no strapping-pin or electrical concerns were involved in that swap.

**Still-unconfirmed values** (called out with comments in `firmware.ino`, not from the schematic):
- MOSFET on/off polarity — assumes each AO3400 is a low-side switch where gate HIGH powers the device on; invert `MOSFET_ON`/`MOSFET_OFF` if wired differently.
- Sensor and DFPlayer power-up settle delays (`SENSOR_WARMUP_MS`, `PLAYER_WARMUP_MS`) — untested guesses.
- Colour-to-track-number mapping in `trackForColour()` — assumes SD card files `0001.mp3`–`0007.mp3` in the order Red, Orange, Yellow, Green, Cyan, Blue, Purple; update once audio files are actually recorded/numbered.

## Electrical Schematic

`electrical/iwand.kicad_pro` + `iwand.kicad_sch` (title block "iWand") is the KiCad schematic this firmware's pin assignments are derived from — treat it as the source of truth, not the pin table above, if the two ever diverge. Symbol reference for firmware pin logic: BT1 (LiPo cell) → U1 (XIAO ESP32S3) → Q1/R1 (sensor power switch) → U2 (GY-33 breakout, symbol `wand_parts:GY33_Breakout`) and Q2/R3 (DFPlayer power switch) → U3 (DFPlayer Mini) → LS1 (speaker), with SW1 as the wake button. No footprints are assigned to any part yet — the schematic notes that U1/U2/U3 must be mounted via female header sockets (removable), and U2's exact module/footprint is still TBD.

## Current State

The firmware is a single-file Arduino sketch (`firmware.ino`). There is no build system, test harness, or additional source file yet, so there are no build/lint/test commands to document. As the sketch grows into multiple files or a PlatformIO project, update this file with the real build/upload/test commands.

## GY-33 Library

Uses the `GY33_Colour_Sensor` Arduino library (https://github.com/You-010/GY33_Arduino), header `GY33.h`, wrapping the TCS34725 chip over I2C or UART. The sketch uses the `GY33_I2C` class.

Confirmed API on `GY33_I2C` (inherited from `GY33_Base` except where noted):
- `begin()` / `begin(TwoWire *wire)` / `begin(int sda, int scl, TwoWire *wire = NULL)`
- `bool update()` — call before reading; returns true when fresh data is available
- `const char* colour()` — named colour (Red, Orange, Yellow, Green, Cyan, Blue, Purple) derived from colour index
- `GY33_Raw getRaw()`, `GY33_Processed getProcessed()`, `GY33_LCC getLCC()` — structured raw/processed/lux+colour-temp readings
- `getZeroed()`, `getCalibrated()`, `calibrateBlack(...)`, `getBlackOffset()`, `setCalibration(...)`, `calibrateWhiteBalance()` — calibration
- `setLED(uint8_t power, bool save = false)`

There are no `red()`/`green()`/`blue()` per-channel accessor methods on this library — per-channel RGB comes from the `GY33_Raw`/`GY33_Processed` structs returned by `getRaw()`/`getProcessed()`, not from individual getters.

## DFPlayer Mini Library

Uses the official `DFRobotDFPlayerMini` Arduino library (https://github.com/DFRobot/DFRobotDFPlayerMini), class `DFRobotDFPlayerMini`, communicating over a `HardwareSerial` (UART) instance rather than I2C.

Confirmed API in use:
- `bool begin(Stream& stream, bool isACK = true, bool doReset = true)`
- `void play(int fileNumber)`, `void volume(uint8_t volume)`
- Playback-finished detection is event-based, not a blocking call: poll `available()`, and when true check `readType() == DFPlayerPlayFinished`.
