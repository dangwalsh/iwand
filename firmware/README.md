# iWand Firmware

Firmware for the iWand color-identifying wand (Seeed XIAO ESP32S3 + GY-33 color sensor + DFPlayer Mini). See `CLAUDE.md` for architecture and pin assignment details.

## Dependencies

Install via the Arduino IDE Library Manager:

- **GY33_Colour_Sensor** — driver for the GY-33 color sensor (TCS34725-based), used over I2C. Source: https://github.com/You-010/GY33_Arduino
- **DFRobotDFPlayerMini** — driver for the DFPlayer Mini MP3 module, used over UART. Source: https://github.com/DFRobot/DFRobotDFPlayerMini

Board support: install the **esp32** board package (Espressif Systems) via Boards Manager, and select an ESP32S3 board profile (e.g. "XIAO_ESP32S3" if available, otherwise a generic "ESP32S3 Dev Module").
