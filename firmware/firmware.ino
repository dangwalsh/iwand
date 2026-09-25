#include <Wire.h>                  // not used directly; GY33.h depends on it
#include <GY33.h>                  // https://github.com/You-010/GY33_Arduino
#include <DFRobotDFPlayerMini.h>   // DFRobot DFPlayer Mini MP3 module
#include <esp_sleep.h>
#include <driver/rtc_io.h>         // RTC-domain pull-up for the wake pin during deep sleep
#include <Preferences.h>           // ESP32 NVS -- persists the black offset across deep sleep

// ---------------------------------------------------------------------------
// Pin assignments -- from the schematic, board is a Seeed XIAO ESP32S3.
// Silkscreen D-labels translated to GPIO numbers per Seeed's pinout:
//   D0=GPIO1  D1=GPIO2  D3=GPIO4  D4=GPIO5  D5=GPIO6  D8=GPIO7  D9=GPIO8
// Sensor MOSFET gate moved from D2 (GPIO3, an ESP32-S3 strapping pin) to
// D0 (GPIO1), a plain unused pin with no competing native function -- update
// the schematic's R1 net from D2 to D0 to match.
// D4/D5 UART roles match the schematic's crossover wiring (D4 = MCU RX, fed
// by DFPlayer TX; D5 = MCU TX, feeding DFPlayer RX).
// D8/D9 are wired to the GY-33's DR/CT pins, which are SDA/SCL in I2C mode
// and RX/TX in serial mode. The sensor runs in serial mode (its factory
// default), so D8 = MCU TX -> GY-33 DR (RX), D9 = MCU RX <- GY-33 CT (TX).
// Grouped for tidiness: D0/D1 = MOSFET gates, D3 = wake button, D4/D5 =
// DFPlayer UART, D8/D9 = GY-33 UART.
// ---------------------------------------------------------------------------
#define SENSOR_MOSFET_PIN  1   // D0: R1 -> Q1 gate, switches GY-33 power
#define PLAYER_MOSFET_PIN  2   // D1: R3 -> Q2 gate, switches DFPlayer power
#define WAKE_BUTTON_PIN    4   // D3: SW1 to GND; wakes MCU via ext0 (active LOW)
#define DFPLAYER_RX_PIN    5   // D4: ESP32 RX <- DFPlayer TXD
#define DFPLAYER_TX_PIN    6   // D5: ESP32 TX -> DFPlayer RXD
#define SENSOR_TX_PIN      7   // D8: ESP32 TX -> GY-33 DR (RX)
#define SENSOR_RX_PIN      8   // D9: ESP32 RX <- GY-33 CT (TX)

// Assumes each AO3400 is wired as a low-side switch (drain to the device's
// ground return, source to system ground): gate HIGH turns the device ON.
// Invert these if the hardware ends up wired the other way.
#define MOSFET_ON  HIGH
#define MOSFET_OFF LOW

#define SENSOR_WARMUP_MS  300   // GY-33 settle time after power-up -- untested guess, tune later
#define SENSOR_BAUD       9600  // GY-33 factory-default serial baud rate
#define PLAYER_WARMUP_MS  2000  // DFPlayer Mini needs time to mount its SD card before it'll respond
#define PLAYER_VOLUME     20    // 0-30
#define READ_TIMEOUT_MS   1000
// In serial mode the GY-33 streams one frame per output type (raw, LCC,
// processed -- all enabled by begin()) and update() returns true for each.
// Waiting for this many frames ensures every field has fresh data.
#define FRAMES_PER_READING 3
// Colour classification thresholds, on channels normalised so the calibrated
// black point is 0 and the white point is 1, then gamma-encoded to match how
// bright things look (see classifyColour()) -- tune on real targets using the
// "Normalised" debug line.
#define DISPLAY_GAMMA        2.2f   // standard sRGB-like perceptual curve
#define BLACK_MAX_VALUE      0.25f  // brightest channel below this -> Black
#define WHITE_MIN_VALUE      0.80f  // unsaturated and at least this bright -> White, else Gray
#define WHITE_MAX_SATURATION 0.15f  // channels this close together -> White/Gray/Black
#define PLAY_TIMEOUT_MS   5000
#define CAL_STEP_TIMEOUT_MS 30000 // abort calibration if SW1 isn't pressed in time, to save battery
#define CAL_SETTLE_MS     500     // time for the GY-33 to finish white balance -- untested guess
#define DEBOUNCE_MS       30
// Debug aid: the XIAO's USB serial drops during deep sleep and the host takes
// a moment to reconnect after each wake, so early prints are lost. Wait up to
// this long for the serial monitor at boot. Set to 0 for battery use.
#define SERIAL_WAIT_MS    3000

HardwareSerial sensorSerial(2);
GY33_UART sensor(sensorSerial);
HardwareSerial dfSerial(1);
DFRobotDFPlayerMini player;
Preferences prefs;

// Raw reading of the white calibration target, loaded from NVS at boot.
GY33_Raw whitePoint = {0, 0, 0, 0};
bool haveWhitePoint = false;

// Maps a classifyColour() name to a track number on the DFPlayer's SD card.
// Track files must exist as 0001.mp3, 0002.mp3, etc. in this same order --
// update once the audio files are actually recorded/numbered.
int trackForColour(const char* colour) {
  if (strcmp(colour, "Black") == 0)  return 1;
  if (strcmp(colour, "White") == 0)  return 2;
  if (strcmp(colour, "Gray") == 0)   return 3;
  if (strcmp(colour, "Red") == 0)    return 4;
  if (strcmp(colour, "Orange") == 0) return 5;
  if (strcmp(colour, "Yellow") == 0) return 6;
  if (strcmp(colour, "Green") == 0)  return 7;
  if (strcmp(colour, "Cyan") == 0)   return 8;
  if (strcmp(colour, "Blue") == 0)   return 9;
  if (strcmp(colour, "Purple") == 0) return 10;
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

void startSensor() {
  powerSensorOn();
  sensorSerial.begin(SENSOR_BAUD, SERIAL_8N1, SENSOR_RX_PIN, SENSOR_TX_PIN);
  sensor.begin(); // enables streaming of raw, LCC and processed frames
}

void stopSensor() {
  // Release the TX pin before cutting the sensor's ground, so the MCU isn't
  // driving a signal into an unpowered module.
  sensorSerial.end();
  powerSensorOff();
}

// Reads frames until every output type has refreshed. Returns false on timeout.
bool refreshSensor() {
  int frames = 0;
  unsigned long start = millis();
  while (millis() - start < READ_TIMEOUT_MS) {
    if (sensor.update() && ++frames >= FRAMES_PER_READING) return true;
  }
  Serial.printf("Sensor read timed out (%d frames received).\n", frames);
  return false;
}

// Scales a raw channel so the black point reads 0 and the white point reads 1,
// then gamma-encodes it. The sensor is linear in light energy, but perceived
// brightness isn't: a mid-tone paper reflecting ~20% of what white paper does
// reads 0.2 linear yet looks about half as bright (0.2^(1/2.2) ~= 0.48).
float normalise(uint16_t raw, uint16_t black, uint16_t white) {
  float span = (white > black) ? (float)(white - black) : 1.0f;
  float linear = (raw > black) ? (raw - black) / span : 0.0f;
  return powf(fminf(linear, 1.0f), 1.0f / DISPLAY_GAMMA);
}

// Names the colour of a raw reading by its hue, saturation and brightness.
// The library's colour() isn't used: it treats the module's colour byte as an
// index 0-7, but the GY-33 reports it as bit flags (e.g. 8 = white), and its
// palette doesn't match our tracks anyway.
const char* classifyColour(const GY33_Raw& raw) {
  GY33_Raw black = sensor.getBlackOffset();
  // Without a white point, scale by the clear channel instead: hue and
  // saturation still work, but brightness (and so Black) can't be judged.
  GY33_Raw white = haveWhitePoint ? whitePoint : GY33_Raw{raw.c, raw.c, raw.c, raw.c};
  float r = normalise(raw.r, black.r, white.r);
  float g = normalise(raw.g, black.g, white.g);
  float b = normalise(raw.b, black.b, white.b);

  float maxc = fmaxf(r, fmaxf(g, b));
  float minc = fminf(r, fminf(g, b));
  float delta = maxc - minc;
  float sat = (maxc > 0) ? delta / maxc : 0;
  float hue = 0;
  if (delta > 0) {
    if (maxc == r)      hue = 60 * fmodf((g - b) / delta, 6);
    else if (maxc == g) hue = 60 * ((b - r) / delta + 2);
    else                hue = 60 * ((r - g) / delta + 4);
    if (hue < 0) hue += 360;
  }
  Serial.printf("Normalised r=%.2f g=%.2f b=%.2f -> hue=%.0f sat=%.2f val=%.2f%s\n",
                r, g, b, hue, sat, maxc, haveWhitePoint ? "" : " (no white point)");

  if (haveWhitePoint && maxc < BLACK_MAX_VALUE) return "Black";
  if (sat < WHITE_MAX_SATURATION) {
    // Without a white point every channel is scaled to the clear channel, so
    // brightness is meaningless and White can't be told from Gray.
    return (!haveWhitePoint || maxc >= WHITE_MIN_VALUE) ? "White" : "Gray";
  }
  // Hue bands in degrees. Measured (gamma-encoded) so far: teal paper ~166,
  // blue ~207, purple ~250. Others are still rough starting points.
  if (hue < 15 || hue >= 330) return "Red";
  if (hue < 40)  return "Orange";
  if (hue < 70)  return "Yellow";
  if (hue < 150) return "Green";
  if (hue < 200) return "Cyan";
  if (hue < 230) return "Blue";
  return "Purple";
}

// Reads the sensor and returns its colour name, or "Unknown" on timeout.
const char* readColour() {
  if (!refreshSensor()) return "Unknown";
  GY33_Raw raw = sensor.getRaw();
  Serial.printf("Sensor read: raw r=%u g=%u b=%u c=%u\n", raw.r, raw.g, raw.b, raw.c);
  return classifyColour(raw);
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

bool buttonHeld() {
  return digitalRead(WAKE_BUTTON_PIN) == LOW;
}

void goToSleep() {
  Serial.println("Going to sleep.");
  Serial.flush();
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_OFF);
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_OFF);
  // ext0 is level-triggered, so sleeping with SW1 still held would wake
  // straight back up and re-run the cycle. Wait for release first.
  while (buttonHeld()) {}
  delay(DEBOUNCE_MS);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BUTTON_PIN, 0); // wake on LOW
  // SW1 has no external pull-up, and pinMode()'s INPUT_PULLUP doesn't apply
  // once the pin is handed to the RTC domain for ext0. Without this the pin
  // floats in deep sleep, reads LOW, and wakes the MCU immediately.
  rtc_gpio_pullup_en((gpio_num_t)WAKE_BUTTON_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)WAKE_BUTTON_PIN);
  esp_deep_sleep_start();
}

// Waits for a full press-and-release of SW1. Returns false on timeout.
bool waitForButtonPress(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!buttonHeld()) {
    if (millis() - start > timeoutMs) return false;
  }
  delay(DEBOUNCE_MS);
  while (buttonHeld()) {}
  delay(DEBOUNCE_MS);
  return true;
}

// Loads the black and white points saved by the last calibration. Both live
// in NVS because RAM (including the library's black offset) is wiped by deep
// sleep.
void loadCalibration() {
  GY33_Raw black;
  prefs.begin("iwand", true);
  if (prefs.getBytes("black", &black, sizeof(black)) == sizeof(black)) {
    sensor.calibrateBlack(black);
  }
  haveWhitePoint = prefs.getBytes("white", &whitePoint, sizeof(whitePoint)) == sizeof(whitePoint);
  prefs.end();
}

void saveCalibrationPoint(const char* key, const GY33_Raw& point) {
  prefs.begin("iwand", false);
  prefs.putBytes(key, &point, sizeof(point));
  prefs.end();
}

// Interactive calibration, entered by holding SW1 at power-up. Expects the
// sensor to already be powered and initialised. Each step waits for a press
// of SW1 with the wand aimed at the appropriate target.
void runCalibration() {
  Serial.println("Calibration mode. Release SW1.");
  while (buttonHeld()) {}
  delay(DEBOUNCE_MS);

  Serial.println("Aim at a WHITE target and press SW1.");
  if (!waitForButtonPress(CAL_STEP_TIMEOUT_MS)) {
    Serial.println("Calibration timed out.");
    return;
  }
  if (!refreshSensor()) {
    Serial.println("Sensor read failed; white point not set.");
    return;
  }
  GY33_Raw white = sensor.getRaw(); // our own white point, used by classifyColour()
  saveCalibrationPoint("white", white);
  // Also calibrate the module's own white balance (affects its processed RGB).
  sensor.calibrateWhiteBalance();
  delay(CAL_SETTLE_MS);
  sensor.saveToFlash(); // keep it across the MOSFET power cuts
  Serial.printf("White point set: r=%u g=%u b=%u c=%u\n", white.r, white.g, white.b, white.c);

  Serial.println("Aim at a BLACK target and press SW1.");
  if (!waitForButtonPress(CAL_STEP_TIMEOUT_MS)) {
    Serial.println("Calibration timed out.");
    return;
  }
  if (!refreshSensor()) {
    Serial.println("Sensor read failed; black point not set.");
    return;
  }
  GY33_Raw black = sensor.calibrateBlack(); // captures the current raw reading
  saveCalibrationPoint("black", black);
  Serial.printf("Black point set: r=%u g=%u b=%u c=%u\n", black.r, black.g, black.b, black.c);

  Serial.println("Calibration complete.");
}

void setup() {
  Serial.begin(115200);
  unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < SERIAL_WAIT_MS) {}
  // Distinguishes a normal button wake from a crash/brownout reset loop.
  Serial.printf("Boot: reset reason %d, wakeup cause %d\n",
                (int)esp_reset_reason(), (int)esp_sleep_get_wakeup_cause());

  pinMode(SENSOR_MOSFET_PIN, OUTPUT);
  pinMode(PLAYER_MOSFET_PIN, OUTPUT);
  digitalWrite(SENSOR_MOSFET_PIN, MOSFET_OFF);
  digitalWrite(PLAYER_MOSFET_PIN, MOSFET_OFF);

  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);

  // SW1 is also the wake button, so it's always pressed on a deep-sleep wake.
  // Only treat it as a calibration request on a cold power-up.
  bool coldBoot = esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0;
  if (coldBoot && buttonHeld()) {
    startSensor();
    runCalibration();
    stopSensor();
    goToSleep();
  }

  // --- Power the sensor, take one reading, power it back down ---
  startSensor();
  loadCalibration();
  const char* detected = readColour();
  stopSensor();

  Serial.print("Detected colour: ");
  Serial.println(detected);

  // --- Power the DFPlayer, announce the colour, power it back down ---
  int track = trackForColour(detected);
  if (track > 0) {
    powerPlayerOn();
    dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX_PIN, DFPLAYER_TX_PIN);
    if (player.begin(dfSerial)) {
      player.volume(PLAYER_VOLUME);
      player.play(track + 1);
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
