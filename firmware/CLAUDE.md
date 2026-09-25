# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

iWand is a hand-held device that identifies the color of an object it's pointed at. This `firmware/` directory holds the embedded code that runs on the device; sibling directories at the repo root (`electrical/`, `mechanical/`) hold the corresponding hardware design work for the same product. `electrical/` has a KiCad project (`iwand.kicad_pro`/`.kicad_sch`) that is the source of truth for pin assignments below; `mechanical/` has a `README.md` with enclosure-relevant context but no design files yet.

## Hardware

- MCU: ESP32-S3
- Sensor: GY-33 color sensor, connected to the ESP32-S3 over UART (the module's serial mode, 9600 baud)
- Audio: 8Ω speaker driven by a DFRobot DFPlayer Mini (UART)
- Power gating: AO3400 N-channel MOSFETs cut power to the GY-33 and the DFPlayer Mini independently when each is not in use, to conserve battery
- Input: a momentary switch wakes the ESP32-S3 from deep sleep

## Architecture

There is no `loop()` logic. The device spends nearly all its time in ESP32-S3 deep sleep; a button press on `WAKE_BUTTON_PIN` triggers an `ext0` wakeup, which resets the MCU and re-enters `setup()` (deep sleep wake is a reset, not a resume — this is why `loop()` is intentionally empty). `setup()` runs the full cycle each time:

1. Power the GY-33 via its MOSFET, take one reading, power it back down.
2. Map the detected colour name to a track number and power the DFPlayer Mini via its MOSFET to play the matching audio file, power it back down once playback finishes (or times out).
3. Re-arm `ext0` wakeup on the button pin and re-enter deep sleep.

SW1 has no external pull-up in the schematic (R2/R4 are MOSFET gate pull-downs). `goToSleep()` must therefore enable the RTC-domain pull-up with `rtc_gpio_pullup_en()`, because `pinMode(INPUT_PULLUP)` doesn't carry into deep sleep. Without it the pin floats, reads LOW, and the device wakes up again straight away in a loop.

**Calibration mode:** holding SW1 during a cold power-up (wakeup cause is not `ext0`, since SW1 is always pressed on a normal wake) runs `runCalibration()` instead of the cycle above: release SW1, aim at white and press SW1, aim at black and press SW1, then sleep. The white step saves the raw reading as the firmware's own white point and also runs the module's `calibrateWhiteBalance()` + `saveToFlash()`. The black step uses `calibrateBlack()`. Each step aborts after `CAL_STEP_TIMEOUT_MS`. Prompts go to Serial only. Both points are stored in NVS (`Preferences`, namespace `iwand`, keys `white` and `black`) and reloaded by `loadCalibration()` on every wake.

**Colour naming:** `classifyColour()` normalises the raw R/G/B so the black point reads 0 and the white point reads 1. It then gamma-encodes the values (`DISPLAY_GAMMA` 2.2), because the sensor's linear readings make colours look far darker than they appear to the eye: coloured papers measured about 0.25 linear. Then it converts to hue/saturation/brightness. Low brightness gives Black. Low saturation gives White or Gray, split by `WHITE_MIN_VALUE`. Otherwise the hue range picks the colour. Measured hues (gamma-encoded): teal paper ~166°, blue ~207°, purple ~250°. The other ranges and thresholds are still starting guesses to tune. Without a saved white point it scales by the clear channel, so hue still works but Black can't be detected.

**Board:** Seeed XIAO ESP32S3. GPIO pin assignments come from the schematic pinout (silkscreen D-labels translated to GPIO numbers per Seeed's pinout table):

| Silkscreen | GPIO | Purpose |
|---|---|---|
| D0 | GPIO1 | R1 → Q1 (AO3400) gate — GY-33 power switch |
| D1 | GPIO2 | R3 → Q2 (AO3400) gate — DFPlayer power switch |
| D3 | GPIO4 | Momentary switch to GND — wake from deep sleep (ext0) |
| D4 | GPIO5 | UART RX from DFPlayer TXD |
| D5 | GPIO6 | UART TX to DFPlayer RXD |
| D8 | GPIO7 | UART TX to GY-33 DR (its RX; SDA in I2C mode) |
| D9 | GPIO8 | UART RX from GY-33 CT (its TX; SCL in I2C mode) |

Pins were deliberately grouped: D0/D1 are the two MOSFET gates, D3 is the wake button, D4/D5 are the DFPlayer UART, D8/D9 are the GY-33 UART. The GY-33 was first driven over I2C on D8/D9 but never answered (`update()` timed out), so it was switched to its factory-default serial mode on the same two wires. The schematic still names them `SDA`/`SCL` (U1 pins `D8_SDA`/`D9_SCL`).

Note: the sensor MOSFET gate was moved from the schematic's original D2 (GPIO3) to D0 (GPIO1), since GPIO3 is one of the ESP32-S3's four strapping pins (0, 3, 45, 46), sampled at boot/reset to configure chip behavior. D0/GPIO1 has no competing native function. The schematic's R1 net and the `XIAO_ESP32S3` symbol's pin 4 have both been updated from `D2` to `D0` to match. The wake button and DFPlayer-MOSFET-gate nets were later swapped between D1 and D3 (schematic net labels `D1`/`D3_BTN`) purely to group pins more logically; no strapping-pin or electrical concerns were involved in that swap. Likewise, I2C and UART were later swapped between D4/D5 and D8/D9, again only for grouping. Both buses go through the ESP32-S3's GPIO matrix, so any of these pins can carry either one. In the schematic, U1's pins 6-9 were renamed `D4_RX`/`D5_TX`/`D8_SDA`/`D9_SCL`, and the `SDA`/`SCL`/`UART_RX`/`UART_TX` labels on U1 were swapped. The labels at the GY-33 and DFPlayer ends were left alone.

**Still-unconfirmed values** (called out with comments in `firmware.ino`, not from the schematic):
- Which of D8/D9 is TX vs RX for the GY-33. This assumes the common GY-33 pinout, where DR is RX/SDA and CT is TX/SCL. Swap `SENSOR_TX_PIN`/`SENSOR_RX_PIN` if no frames arrive.
- Sensor and DFPlayer power-up settle delays (`SENSOR_WARMUP_MS`, `PLAYER_WARMUP_MS`) — untested guesses.
- Colour-to-track-number mapping in `trackForColour()` — assumes SD card files `0001.mp3`–`0007.mp3` in the order Red, Orange, Yellow, Green, Cyan, Blue, Purple; update once audio files are actually recorded/numbered.

## Electrical Schematic

`electrical/iwand.kicad_pro` + `iwand.kicad_sch` (title block "iWand") is the KiCad schematic this firmware's pin assignments are derived from — treat it as the source of truth, not the pin table above, if the two ever diverge. Symbol reference for firmware pin logic: BT1 (LiPo cell) → U1 (XIAO ESP32S3) → Q1/R1 (sensor power switch) → U2 (GY-33 breakout, symbol `wand_parts:GY33_Breakout`) and Q2/R3 (DFPlayer power switch) → U3 (DFPlayer Mini) → LS1 (speaker), with SW1 as the wake button. No footprints are assigned to any part yet — the schematic notes that U1/U2/U3 must be mounted via female header sockets (removable), and U2's exact module/footprint is still TBD.

## Current State

The firmware is a single-file Arduino sketch (`firmware.ino`). There is no build system, test harness, or additional source file yet, so there are no build/lint/test commands to document. As the sketch grows into multiple files or a PlatformIO project, update this file with the real build/upload/test commands.

## GY-33 Library

Uses the `GY33_Colour_Sensor` Arduino library (https://github.com/You-010/GY33_Arduino), header `GY33.h`, wrapping the TCS34725 chip over I2C or UART. The sketch uses the `GY33_UART` class on `HardwareSerial(2)`; `HardwareSerial(1)` is the DFPlayer.

`GY33_UART` specifics (from the library source):
- The constructor is `GY33_UART(Stream &serial)`. The caller must call `sensorSerial.begin(baud, SERIAL_8N1, rx, tx)` first. `begin()` only sends the command that turns on streaming of processed, LCC and raw frames.
- `update()` parses a continuous incoming stream and returns true after **each** complete frame, whatever its type. That's why `refreshSensor()` waits for `FRAMES_PER_READING` (3) frames before any fields are read.
- `calibrateWhiteBalance()` sends command `0xA5 0xBB`. `saveToFlash()` (`0xA5 0xCC`) saves settings in the module; calibration calls it after white balance so the result survives MOSFET power cuts.

Confirmed API shared with `GY33_I2C` (from `GY33_Base`):
- `const char* colour()` — named colour (Red, Orange, Yellow, Green, Cyan, Blue, Purple) derived from colour index
- `GY33_Raw getRaw()`, `GY33_Processed getProcessed()`, `GY33_LCC getLCC()` — structured raw/processed/lux+colour-temp readings
- `getZeroed()`, `getCalibrated()`, `calibrateBlack(...)`, `getBlackOffset()`, `setCalibration(...)`, `calibrateWhiteBalance()` — calibration
- `setLED(uint8_t power, bool save = false)`

Calibration behaviour (from the library source):
- `calibrateWhiteBalance()` is done by the GY-33 module's own MCU, both over I2C (sets bit 0 of register 0x10) and over UART.
- `calibrateBlack()` just copies the last `update()`'s raw reading into a RAM-only `_blackOffset` and returns it. `calibrateBlack(const GY33_Raw&)` sets it directly. The offset only affects `getZeroed()`/`getCalibrated()`.
- `colour()` returns the module's own colour byte (`_lcc.colourIndex`) as an index 0-7 and gives "Unknown" above 7. On real hardware the byte came back as `8` for light-blue paper. It looks like the GY-33 reports it as bit flags (8 = white, per the datasheet as remembered — not confirmed), so the sketch doesn't use `colour()`. See `classifyColour()`.

There are no `red()`/`green()`/`blue()` per-channel accessor methods on this library — per-channel RGB comes from the `GY33_Raw`/`GY33_Processed` structs returned by `getRaw()`/`getProcessed()`, not from individual getters.

## DFPlayer Mini Library

Uses the official `DFRobotDFPlayerMini` Arduino library (https://github.com/DFRobot/DFRobotDFPlayerMini), class `DFRobotDFPlayerMini`, communicating over a `HardwareSerial` (UART) instance rather than I2C.

Confirmed API in use:
- `bool begin(Stream& stream, bool isACK = true, bool doReset = true)`
- `void play(int fileNumber)`, `void volume(uint8_t volume)`
- Playback-finished detection is event-based, not a blocking call: poll `available()`, and when true check `readType() == DFPlayerPlayFinished`.
