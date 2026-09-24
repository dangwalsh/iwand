#include <Wire.h>
#include <GY33.h>                  // https://github.com/You-010/GY33_Arduino
#include <DFRobotDFPlayerMini.h>   // DFRobot DFPlayer Mini MP3 module
#include <esp_sleep.h>

// ---------------------------------------------------------------------------
// Pin assignments -- from the schematic, board is a Seeed XIAO ESP32S3.
// Silkscreen D-labels translated to GPIO numbers per Seeed's pinout:
//   D0=GPIO1  D1=GPIO2  D3=GPIO4  D4=GPIO5  D5=GPIO6  D8=GPIO7  D9=GPIO8
// Sensor MOSFET gate moved from D2 (GPIO3, an ESP32-S3 strapping pin) to
// D0 (GPIO1), a plain unused pin with no competing native function -- update
// the schematic's R1 net from D2 to D0 to match.
// D8/D9 UART roles match the schematic's crossover wiring (D8 = MCU RX, fed
// by DFPlayer TX; D9 = MCU TX, feeding DFPlayer RX).
// ---------------------------------------------------------------------------
#define WAKE_BUTTON_PIN    2   // D1: SW1 to GND; wakes MCU via ext0 (active LOW)
#define SENSOR_MOSFET_PIN  1   // D0: R1 -> Q1 gate, switches GY-33 power
#define PLAYER_MOSFET_PIN  4   // D3: R3 -> Q2 gate, switches DFPlayer power
#define I2C_SDA_PIN        5   // D4: TCS34725 SDA
#define I2C_SCL_PIN        6   // D5: TCS34725 SCL
#define DFPLAYER_RX_PIN    7   // D8: ESP32 RX <- DFPlayer TXD
#define DFPLAYER_TX_PIN    8   // D9: ESP32 TX -> DFPlayer RXD

// Assumes each AO3400 is wired as a low-side switch (drain to the device's
// ground return, source to system ground): gate HIGH turns the device ON.
// Invert these if the hardware ends up wired the other way.
#define MOSFET_ON  HIGH
#define MOSFET_OFF LOW

#define SENSOR_WARMUP_MS  50    // GY-33 settle time after power-up -- untested guess, tune later
#define PLAYER_WARMUP_MS  2000  // DFPlayer Mini needs time to mount its SD card before it'll respond
#define PLAYER_VOLUME     20    // 0-30
#define READ_TIMEOUT_MS   1000
#define PLAY_TIMEOUT_MS   5000

GY33_I2C sensor;
HardwareSerial dfSerial(1);
DFRobotDFPlayerMini player;

// Maps a GY33 colour() name to a track number on the DFPlayer's SD card.
// Track files must exist as 0001.mp3, 0002.mp3, etc. in this same order --
// update once the audio files are actually recorded/numbered.
int trackForColour(const char* colour) {
  if (strcmp(colour, "Red") == 0)    return 1;
  if (strcmp(colour, "Orange") == 0) return 2;
  if (strcmp(colour, "Yellow") == 0) return 3;
  if (strcmp(colour, "Green") == 0)  return 4;
  if (strcmp(colour, "Cyan") == 0)   return 5;
  if (strcmp(colour, "Blue") == 0)   return 6;
  if (strcmp(colour, "Purple") == 0) return 7;
  return 0; // unrecognised -- don't play anything
}

void powerSensorOn() {
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_ON);
  delay(SENSOR_WARMUP_MS);
}

void powerSensorOff() {
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_OFF);
}

void powerPlayerOn() {
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_ON);
  delay(PLAYER_WARMUP_MS);
}

void powerPlayerOff() {
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_OFF);
}

// Reads the sensor and returns its colour name, or "Unknown" on timeout.
const char* readColour() {
  unsigned long start = millis();
  while (millis() - start < READ_TIMEOUT_MS) {
    if (sensor.update()) {
      return sensor.colour();
    }
  }
  return "Unknown";
}

// Blocks until the DFPlayer reports the current track finished, or times out.
void waitForPlaybackToFinish() {
  unsigned long start = millis();
  while (millis() - start < PLAY_TIMEOUT_MS) {
    if (player.available() && player.readType() == DFPlayerPlayFinished) {
      return;
    }
  }
  Serial.println("Playback timed out.");
}

void goToSleep() {
  Serial.println("Going to sleep.");
  Serial.flush();
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_OFF);
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_OFF);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BUTTON_PIN, 0); // wake on LOW
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_MOSFET_PIN, OUTPUT);
  pinMode(PLAYER_MOSFET_PIN, OUTPUT);
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_OFF);
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_OFF);

  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

  // --- Power the sensor, take one reading, power it back down ---
  powerSensorOn();
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  sensor.begin(&Wire);
  const char* detected = readColour();
  powerSensorOff();

  Serial.print("Detected colour: ");
  Serial.println(detected);

  // --- Power the DFPlayer, announce the colour, power it back down ---
  int track = trackForColour(detected);
  if (track > 0) {
    powerPlayerOn();
    dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    if (player.begin(dfSerial)) {
      player.volume(PLAYER_VOLUME);
      player.play(track);
      waitForPlaybackToFinish();
    } else {
      Serial.println("DFPlayer init failed.");
    }
    powerPlayerOff();
  }

  goToSleep();
}

void loop() {
  // Unused: deep sleep resets the MCU on wake, so execution always restarts
  // from setup() rather than resuming loop().
}
