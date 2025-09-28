#include <Wire.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>      // for ESP.restart()

// === CONFIG ===
#define I2C_ADDR        0x10   // set per board: 0x10=WEST, 0x11=SOUTH, 0x12=EAST, 0x13=NORTH
#define SDA_PIN         6
#define SCL_PIN         7

#define CMD_READ_COUNT  0x01
#define CMD_READ_DATA   0x02

#define MAX_DEVICES     4      // How many records we'll pack at most

// ==== BLE SCAN CONFIG ====
const uint32_t SCAN_DURATION = 1;      // seconds
const uint16_t SCAN_WINDOW   = 0x50;   // ≈31 ms
const uint16_t SCAN_INTERVAL = 0x50;   // ≈31 ms
BLEScan* pBLEScan;

// ==== FILTER CONFIG ====
// This will allow anything that matches the OUI filter or full MAC address filter.
// allowed OUIs (first 3 bytes)
const char* ALLOWED_OUIS[] = {
  "AA:BB:CC",
  "AA:BB:CC",
  "AA:BB:CC"
};
const size_t NUM_OUIS = sizeof(ALLOWED_OUIS) / sizeof(ALLOWED_OUIS[0]);

// Allowed full MACs
const char* ALLOWED_MACS[] = {
  "AA:BB:CC:11:22:33",
  "AA:BB:CC:11:22:33" 
};
const size_t NUM_MACS = sizeof(ALLOWED_MACS) / sizeof(ALLOWED_MACS[0]);

// ==== STATE & BUFFERS ====
// Scratch storage for up to MAX_DEVICES
uint8_t  macs[MAX_DEVICES][6];
int8_t   rssis[MAX_DEVICES];
size_t   foundCount = 0;

// txBuf = [ count ][ MAC6, RSSI1 ] × foundCount
uint8_t txBuf[1 + MAX_DEVICES * 7];
size_t  txLen = 1;

volatile uint8_t lastCmd = 0;

// ==== HELPERS ====

// Uppercase a String
String toUpperStr(const String& s) {
  String out = s;
  out.toUpperCase();
  return out;
}

// Return true if this MAC matches one of the allowed lists
bool isAllowedDevice(const String& mac) {
  String M = toUpperStr(mac);
  // Full MAC
  for (size_t i = 0; i < NUM_MACS; i++) {
    if (M == ALLOWED_MACS[i]) return true;
  }
  // OUI prefix
  String pr = M.substring(0, 8);  // "XX:XX:XX"
  for (size_t i = 0; i < NUM_OUIS; i++) {
    if (pr == ALLOWED_OUIS[i]) return true;
  }
  return false;
}

// ==== I2C CALLBACKS ====

void onI2CReceive(int byteCount) {
  if (Wire.available()) {
    lastCmd = Wire.read();
  }
}

void onI2CRequest() {
  if (lastCmd == CMD_READ_COUNT) {
    // Just send the count byte
    Wire.write(txBuf, 1);
  } else if (lastCmd == CMD_READ_DATA) {
    // Send everything after the count
    Wire.write(txBuf + 1, txLen - 1);
  } else {
    // If an unknown command, send zero
    uint8_t z = 0;
    Wire.write(&z, 1);
  }
}

// ==== BUILD THE BUFFER ====

void prepareBuffer() {
  // Scan
  BLEScanResults* results = pBLEScan->start(SCAN_DURATION, false);
  int total = results->getCount();
  foundCount = 0;

  // Walk through results and apply filter
  for (int i = 0; i < total && foundCount < MAX_DEVICES; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    String macStr = dev.getAddress().toString();
    if (!isAllowedDevice(macStr)) continue;

    // Copy raw 6-byte MAC + RSSI
    auto raw = dev.getAddress().getNative();  // uint8_t[6]
    memcpy(macs[foundCount], raw, 6);
    rssis[foundCount] = dev.getRSSI();
    foundCount++;
  }

  // Pack into txBuf
  txBuf[0] = (uint8_t)foundCount;
  size_t pos = 1;
  for (size_t i = 0; i < foundCount; i++) {
    memcpy(txBuf + pos, macs[i], 6);
    pos += 6;
    txBuf[pos++] = (uint8_t)rssis[i];
  }
  txLen = pos;

  pBLEScan->clearResults();
}

// ==== SETUP & LOOP ====

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // BLE init + Scan parameters
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(false);
  pBLEScan->setWindow(SCAN_WINDOW);
  pBLEScan->setInterval(SCAN_INTERVAL);

  // I2C slave
  Wire.begin(I2C_ADDR, SDA_PIN, SCL_PIN, 100000);
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  Serial.printf("Scanner @0x%02X ready\n", I2C_ADDR);
}

void loop() {
  prepareBuffer();
  delay(50);  // Give master time to poll
}
