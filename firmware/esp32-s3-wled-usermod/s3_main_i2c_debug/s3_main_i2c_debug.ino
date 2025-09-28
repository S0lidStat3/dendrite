#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// ——————————————————————————————
// PIN & BUS CONFIGURATION
// ——————————————————————————————
#define SDA_PIN     4     // I2C SDA on ESP32-S3
#define SCL_PIN     5     // I2C SCL on ESP32-S3

// NeoPixel (RGB) on GPIO48
#define RGB_PIN     48
#define BRIGHTNESS  50    // 0-255

#define i2cbuflen 32

// One NeoPixel = 1 LED on the board
Adafruit_NeoPixel strip(1, RGB_PIN, NEO_GRB + NEO_KHZ800);

// ——————————————————————————————
// SCANNER BOARD DEFINITIONS
// ——————————————————————————————
struct ScannerBoard {
  uint8_t   address;    // I²C address
  const char* label;    // human-readable
};
ScannerBoard boards[] = {
  { 0x10, "WEST"  },
  { 0x11, "SOUTH" },
  { 0x12, "EAST"  },
  { 0x13, "NORTH" }
};

// ——————————————————————————————
// HELPER: blink the NeoPixel a solid color
// ——————————————————————————————
void blinkColor(uint8_t r, uint8_t g, uint8_t b, int duration_ms) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  delay(duration_ms);
  strip.clear();
  strip.show();
}

// ——————————————————————————————
// SETUP()
// ——————————————————————————————
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Debugger started");

  // Initialize I²C as master on SDA=4, SCL=5
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Initialize NeoPixel for visual feedback
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();

  // (Optional) Startup sequence: white>red>green
  blinkColor(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS, 200);
  delay(100);
  blinkColor(BRIGHTNESS, 0, 0, 200);
  delay(100);
  blinkColor(0, BRIGHTNESS, 0, 200);
  delay(100);
}

// ——————————————————————————————
// MAIN LOOP()
// ——————————————————————————————
void loop() {
  // For each of the four scanner boards:
  for (auto &b : boards) {
  // 1) Print which board we're polling
    Serial.printf("Polling %s (%02#X): ", b.label, b.address);

  // 2) Send a zero-byte I2C "ping" to that address
    Wire.beginTransmission(b.address);
    bool ack = (Wire.endTransmission() == 0);

    if (ack) {
      // • If we got an ACK, light the NeoPixel GREEN for 300 ms
      // • Then request up to 128 bytes of data
      blinkColor(0, BRIGHTNESS, 0, 300);

      int returned = Wire.requestFrom(b.address, i2cbuflen);
      Serial.printf(" (receiving %00d bytes) ", returned);
      String s;
      unsigned long start = millis();
      while (Wire.available() && millis() - start < 100) {
        s += char(Wire.read());
      }
      Serial.printf("OK, data= %s\n", s);
    } else {xxxx
      // • If we got no ACK (NACK), blink RED for 100 ms
      Serial.println("no ack");
      blinkColor(BRIGHTNESS, 0, 0, 100);
    }

    delay(100);  // brief pause between boards
  }

  Serial.println("--- done ---\n");
  delay(1000);   // wait before the next full polling cycle
}
