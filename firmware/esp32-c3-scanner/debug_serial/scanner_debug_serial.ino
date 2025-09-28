#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_system.h>  

// ==== CONFIGURATION ====
const char* DIRECTION_LABEL    = "NORTH"; // Change for each board (NORTH, EAST, SOUTH, WEST). I used directions early on in development and they stuck around. The production version uses I2C addresses for the usermod.
const uint32_t SCAN_DURATION   = 1;     // seconds
const uint16_t SCAN_WINDOW     = 0x50;  // ≈31 ms
const uint16_t SCAN_INTERVAL   = 0x50;  // ≈31 ms
const bool FILTER_BY_OUI       = true;  // toggle OUI filtering on/off

// OUIs to match (first 3 bytes of MAC address, colon-separated, uppercase)
const char* OUIS[] = {
  "AA:BB:CC"
};
const size_t NUM_OUIS = sizeof(OUIS) / sizeof(OUIS[0]);

BLEScan* pBLEScan;

bool startsWithOUI(String mac) {
  mac.toUpperCase();  // Normalize to uppercase for matching
  for (size_t i = 0; i < NUM_OUIS; i++) {
    if (mac.startsWith(OUIS[i])) {
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(false);
  pBLEScan->setWindow(SCAN_WINDOW);
  pBLEScan->setInterval(SCAN_INTERVAL);
}

void loop() {
  // Reboot command  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "REBOOT") {
      Serial.println("Received REBOOT command, restarting...");
      ESP.restart();
    }
  }

  // Header
  Serial.print("Starting BLE scan... [");
  Serial.print(DIRECTION_LABEL);
  Serial.print("] | ");
  Serial.println(millis());

  // Start scan (returns pointer in this library version)
  BLEScanResults* results = pBLEScan->start((uint32_t)SCAN_DURATION, false);

  // Print every result (no dedupe)
  int count = results->getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice dev = results->getDevice(i);
    String mac = String(dev.getAddress().toString().c_str());

    if (!FILTER_BY_OUI || startsWithOUI(mac)) {
      Serial.print("MAC: ");
      Serial.print(mac);
      Serial.print(" | RSSI: ");
      Serial.println(dev.getRSSI());
    }
  }

  // Done
  Serial.println("Scan complete");
  pBLEScan->clearResults();
}