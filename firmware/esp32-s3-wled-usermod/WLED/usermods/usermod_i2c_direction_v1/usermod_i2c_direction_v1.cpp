#pragma once
#include "wled.h"
#include <Wire.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

// =========================
// Config (tweak as needed)
// =========================
// MAC address and OUI filters are set in scanner firmware to minimize data over the I2C bus.
// Debug prints (0 = off, 1 = on)
#ifndef I2C_DIR_DEBUG
#define I2C_DIR_DEBUG 0
#endif

// Serial sniffing of MAC + per-antenna RSSI (0=off, 1=on)
#ifndef I2C_DIR_SNIFF
#define I2C_DIR_SNIFF 0
#endif

// Minimum number of antennas required to trust a device this frame
#ifndef MIN_VALID_ANTENNAS
#define MIN_VALID_ANTENNAS 4   // If you want to run this with fewer scanner boards, you can lower this. But it will have an effect on direction finding accuracy.
#endif

// I2C pins on ESP32-S3
#define I2C_SDA 4
#define I2C_SCL 5
#ifndef I2C_CLK
#define I2C_CLK 100000
#endif

// Scanner addresses in fixed orientation order: [W, S, E, N]
static const uint8_t I2C_ADDRS[4] = {0x10, 0x11, 0x12, 0x13};

#define CMD_READ_COUNT 0x01
#define CMD_READ_DATA  0x02
#define RECORD_SIZE    7

#ifndef SLOT_PERIOD_MS
#define SLOT_PERIOD_MS 100
#endif

#define HELMET_LEN         41
#define FRONT_LED_1BASED   20
#define RIGHT_LED_1BASED   10
#define BACK_LED_1BASED    41
#define LEFT_LED_1BASED    30

#ifndef HEADING_OFFSET_DEG
#define HEADING_OFFSET_DEG 0.0f
#endif

#ifndef TARGET_SEGMENT
#define TARGET_SEGMENT 255
#endif

#ifndef MARKER_WIDTH_PX
#define MARKER_WIDTH_PX 5
#endif
#ifndef MARKER_COLOR
#define MARKER_COLOR RGBW32(255,255,255,0)
#endif

#ifndef TOP_K_DEVICES
#define TOP_K_DEVICES 32
#endif

#ifndef AUTO_PRESET_ENABLE
#define AUTO_PRESET_ENABLE        1
#endif
#ifndef PRESET_ON_DETECTED
#define PRESET_ON_DETECTED        1
#endif
#ifndef PRESET_ON_IDLE
#define PRESET_ON_IDLE            2
#endif
#ifndef PRESENCE_HYSTERESIS_MS
#define PRESENCE_HYSTERESIS_MS  3000
#endif
#ifndef SWITCH_MIN_INTERVAL_MS
#define SWITCH_MIN_INTERVAL_MS  5000
#endif

#if I2C_DIR_DEBUG
  #define DBG(...)  do { Serial.printf(__VA_ARGS__); } while (0)
  #define DBGLN(...) do { Serial.println(__VA_ARGS__); } while (0)
#else
  #define DBG(...)
  #define DBGLN(...)
#endif

static inline float wrap360(float deg) {
  while (deg < 0.0f)  deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}
static inline float linFromDb(int8_t db) { return powf(10.0f, (float)db / 10.0f); }

static inline uint16_t angleToLocalIdx41(float aCW) {
  float a = wrap360(aCW);
  float t;
  if (a < 90.0f)       t = a / 9.0f;
  else if (a < 180.0f) t = 10.0f + (a - 90.0f) / 9.0f;
  else if (a < 270.0f) t = 20.0f + (a - 180.0f) * (11.0f / 90.0f);
  else                 t = 31.0f + (a - 270.0f) / 9.0f;

  uint16_t steps = (uint16_t)floorf(t + 1e-6f);
  int led1 = FRONT_LED_1BASED - (int)steps;
  while (led1 < 1)  led1 += HELMET_LEN;
  while (led1 > HELMET_LEN) led1 -= HELMET_LEN;
  return (uint16_t)(led1 - 1);
}

// =========================
// Calibration table [antenna][angle_bucket]
// This is a calibration table for my specific antennas, which are different than yours.
// Please make your own calibration table with the provided calibration logger and readme.
// =========================
float calibration[4][36] = {
  // West
  { 6.470f, 2.283f, 1.988f, -0.571f, -2.056f, -4.296f, -5.963f, -8.123f, -6.290f, -7.227f, -6.638f, -8.733f, -7.528f, -6.890f, -6.041f, -8.680f, -9.120f, -9.946f, -4.050f, -5.711f, -5.578f, -1.941f, -0.945f, 1.208f, 3.546f, 4.156f, 6.480f, 9.294f, 9.882f, 8.066f, 8.522f, 8.366f, 8.327f, 9.606f, 5.563f, 7.758f },
  // South
  { 5.473f, 7.002f, 8.649f, 9.491f, 9.283f, 9.478f, 8.360f, 9.058f, 8.355f, 6.618f, 4.825f, 2.653f, -0.660f, 0.577f, -0.893f, -3.758f, -2.504f, -3.661f, -7.121f, -7.730f, -5.864f, -7.611f, -9.556f, -11.550f, -8.422f, -7.273f, -8.515f, -8.502f, -5.519f, -3.926f, -3.298f, 0.220f, -1.372f, 1.812f, 3.140f, 5.217f },
  // East
  { -5.054f, -1.544f, -5.455f, -4.509f, -1.856f, 2.189f, 2.728f, 6.253f, 7.212f, 11.335f, 10.282f, 12.088f, 11.107f, 12.107f, 9.926f, 10.263f, 10.773f, 9.892f, 7.929f, 9.318f, 9.162f, 8.552f, 10.745f, 10.342f, 8.651f, 7.598f, 4.090f, 0.877f, -0.530f, -3.024f, -4.314f, -5.084f, -6.955f, -7.134f, -5.412f, -7.493f },
  // North
  { -6.889f, -7.741f, -5.182f, -4.411f, -5.371f, -7.370f, -5.125f, -7.188f, -9.277f, -10.726f, -8.469f, -6.009f, -2.919f, -5.793f, -2.992f, 2.175f, 0.851f, 3.715f, 3.242f, 4.123f, 2.280f, 0.999f, -0.244f, 0.000f, -3.775f, -4.481f, -2.055f, -1.669f, -3.833f, -1.116f, -0.911f, -3.502f, 0.000f, -4.284f, -3.291f, -5.482f }
};

// =========================
// Data structures
// =========================
struct Device {
  uint8_t mac[6];
  int8_t  rssi[4];
  bool    seen;
};

class UsermodI2CDirectionV2 : public Usermod {
private:
  bool initDone = false;
  int8_t targetSegId = -1;
  volatile uint64_t overlayBits = 0;
  volatile bool presenceAtomic = false;
  portMUX_TYPE overlayMux = portMUX_INITIALIZER_UNLOCKED;
  bool presenceLast = false;
  unsigned long presenceChangedAt = 0;
  unsigned long lastPresetSwitchAt = 0;
  uint16_t currentPresetApplied = 0;
  TaskHandle_t pollTaskHandle = nullptr;

  int8_t resolveTargetSegment() {
    if (targetSegId >= 0) return targetSegId;
    if (TARGET_SEGMENT != 255) { targetSegId = (int8_t)TARGET_SEGMENT; return targetSegId; }
    uint8_t nSeg = strip.getSegmentsNum();
    for (uint8_t i = 0; i < nSeg; i++) {
      Segment &s = strip.getSegment(i);
      if (s.stop > s.start && (s.stop - s.start) == HELMET_LEN) { targetSegId = (int8_t)i; return targetSegId; }
    }
    targetSegId = (int8_t)strip.getMainSegmentId();
    return targetSegId;
  }

  bool readCount(uint8_t addr, uint8_t &countOut) {
    Wire.beginTransmission(addr);
    Wire.write(CMD_READ_COUNT);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
    countOut = Wire.read();
    return true;
  }

  // Helper: print MAC + RSSIs [W,S,E,N]
  static inline void printDeviceRssi(const Device& d) {
  #if I2C_DIR_SNIFF
    // Format MAC
    char macbuf[18];
    snprintf(macbuf, sizeof(macbuf), "%02X:%02X:%02X:%02X:%02X:%02X",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    // Note: RSSIs are raw (pre-calibration) for sanity checks
    Serial.printf("[RSSI] MAC %s  W:%d  S:%d  E:%d  N:%d\r\n",
                  macbuf, (int)d.rssi[0], (int)d.rssi[1], (int)d.rssi[2], (int)d.rssi[3]);
  #endif
  }

  // === Partial-antenna helpers ===
  static inline bool antValid(const Device& d, int i) { return d.rssi[i] != -128; }

  // Compute vx,vy using only available antennas.
  // If both in a pair exist -> use difference. If only one exists -> use its signed contribution.
  static inline bool computeVectorPartial(const Device& d, float &vx, float &vy, int &nAxes) {
    vx = 0.0f; vy = 0.0f; nAxes = 0;

    const bool hasW = antValid(d,0), hasS = antValid(d,1), hasE = antValid(d,2), hasN = antValid(d,3);

    const float pW = hasW ? linFromDb(d.rssi[0]) : 0.0f;
    const float pS = hasS ? linFromDb(d.rssi[1]) : 0.0f;
    const float pE = hasE ? linFromDb(d.rssi[2]) : 0.0f;
    const float pN = hasN ? linFromDb(d.rssi[3]) : 0.0f;

    // X-axis (E-W)
    if (hasE && hasW) { vx += (pE - pW); nAxes++; }
    else if (hasE && !hasW) { vx += pE; nAxes++; }
    else if (!hasE && hasW) { vx -= pW; nAxes++; }

    // Y-axis (N-S)
    if (hasN && hasS) { vy += (pN - pS); nAxes++; }
    else if (hasN && !hasS) { vy += pN; nAxes++; }
    else if (!hasN && hasS) { vy -= pS; nAxes++; }

    return (nAxes > 0);
  }

  static inline int countValidAntennas(const Device& d) {
    int c = 0; for (int i=0;i<4;i++) if (antValid(d,i)) c++; return c;
  }

  void pollTask() {
    std::vector<Device> devices; devices.reserve(256);
    auto getOrCreate = [&](const uint8_t mac[6]) -> Device& {
      for (auto &d : devices) if (memcmp(d.mac, mac, 6) == 0) return d;
      Device nd; memcpy(nd.mac, mac, 6); for (int i=0;i<4;i++) nd.rssi[i] = -128; nd.seen=false;
      devices.push_back(nd); return devices.back();
    };

    uint8_t curSlot = 0;
    uint8_t slotsMask = 0;
    unsigned long lastTick = 0;

    for (;;) {
      unsigned long now = millis();
      if (now - lastTick < SLOT_PERIOD_MS) { vTaskDelay(pdMS_TO_TICKS(5)); continue; }
      lastTick = now;

      uint8_t addr = I2C_ADDRS[curSlot];
      uint8_t count = 0;
      if (readCount(addr, count) && count) {
        size_t bytesToRead = (size_t)count * RECORD_SIZE;
        Wire.beginTransmission(addr);
        Wire.write(CMD_READ_DATA);
        if (Wire.endTransmission() == 0) {
          int got = Wire.requestFrom(addr, (uint8_t)bytesToRead);
          if (got >= (int)bytesToRead) {
            for (uint8_t i=0;i<count;i++) {
              uint8_t mac[6]; for (int j=0;j<6;j++) mac[j]=Wire.read();
              int8_t rssi = (int8_t)Wire.read();
              auto &d = getOrCreate(mac);
              d.rssi[curSlot] = rssi; d.seen = true;
            }
          }
        }
      }

      slotsMask |= (1 << curSlot);
      curSlot = (curSlot + 1) & 0x03;

      if (slotsMask == 0x0F) {
        slotsMask = 0;

        // === Serial sniff: print raw per-antenna RSSI for each device (pre-calibration) ===
      #if I2C_DIR_SNIFF
        for (const auto &d : devices) {
          printDeviceRssi(d);
        }
      #endif

        // === Apply calibration (supports fewer/missing antennas if needed, at the cost of accuracy.) ===
        for (auto &d : devices) {
          if (countValidAntennas(d) < MIN_VALID_ANTENNAS) continue;

          float vx, vy; int nAxes;
          if (!computeVectorPartial(d, vx, vy, nAxes)) continue;

          float angCCW = atan2f(vy, vx) * 180.0f / PI;       // CCW from +X
          angCCW = wrap360(angCCW);
          float aCW_fromFront = wrap360(HEADING_OFFSET_DEG - angCCW);
          int bucket = (int)lroundf(aCW_fromFront / 10.0f) % 36;

          for (int ant = 0; ant < 4; ant++) {
            if (antValid(d, ant)) d.rssi[ant] -= calibration[ant][bucket];
          }
        }

        // === Scoring using average power over present antennas ===
        struct ScoredIdx { uint16_t idx; float score; };
        std::vector<ScoredIdx> scores; scores.reserve(devices.size());
        for (uint16_t i=0; i<devices.size(); i++) {
          auto &d = devices[i];
          int k = countValidAntennas(d);
          if (k < MIN_VALID_ANTENNAS) continue;

          float sumP = 0.0f;
          for (int a=0;a<4;a++) if (antValid(d,a)) sumP += linFromDb(d.rssi[a]);
          float s = (k > 0) ? (sumP / (float)k) : 0.0f;
          scores.push_back({i, s});
        }
        if (TOP_K_DEVICES > 0 && scores.size() > (size_t)TOP_K_DEVICES) {
          auto nth = scores.begin() + TOP_K_DEVICES;
          std::nth_element(scores.begin(), nth, scores.end(),
                           [](const ScoredIdx&a,const ScoredIdx&b){return a.score>b.score;});
          scores.resize(TOP_K_DEVICES);
        }

        uint64_t newBits = 0;
        for (auto &si : scores) {
          auto &d = devices[si.idx];

          float vx, vy; int nAxes;
          if (!computeVectorPartial(d, vx, vy, nAxes)) continue;

          float angCCW = atan2f(vy, vx) * 180.0f / PI;
          angCCW = wrap360(angCCW);
          float aCW_fromFront = wrap360(HEADING_OFFSET_DEG - angCCW);
          uint16_t idx = angleToLocalIdx41(aCW_fromFront);

          const int half = MARKER_WIDTH_PX / 2;
          for (int dpx = -half; dpx <= half; dpx++) {
            int i = (int)idx + dpx;
            if (i < 0) i += HELMET_LEN; else if (i >= HELMET_LEN) i -= HELMET_LEN;
            newBits |= (1ULL << i);
          }
        }

        bool any = (newBits != 0);
        portENTER_CRITICAL(&overlayMux);
        overlayBits = newBits;
        presenceAtomic = any;
        portEXIT_CRITICAL(&overlayMux);

        devices.clear();
      }
    }
  }

  void maybeSwitchPreset(bool presenceNow) {
#if AUTO_PRESET_ENABLE
    unsigned long now = millis();
    if (presenceNow != presenceLast) {
      presenceLast = presenceNow;
      presenceChangedAt = now;
      return;
    }
    if (now - presenceChangedAt < PRESENCE_HYSTERESIS_MS) return;
    if (now - lastPresetSwitchAt < SWITCH_MIN_INTERVAL_MS) return;
    uint16_t desired = presenceNow ? PRESET_ON_DETECTED : PRESET_ON_IDLE;
    if (desired == currentPresetApplied) return;
    applyPreset(desired);
    currentPresetApplied = desired;
    lastPresetSwitchAt = now;
#endif
  }

public:
  void setup() override {
    initDone = true;
#if I2C_DIR_DEBUG || I2C_DIR_SNIFF
    Serial.begin(115200);
    delay(50);
#endif
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLK);
    Wire.setTimeOut(10);
    xTaskCreatePinnedToCore(
      [](void* arg){ static_cast<UsermodI2CDirectionV2*>(arg)->pollTask(); },
      "i2cdir", 8192, this, 1, &pollTaskHandle, 0
    );
  }

  void loop() override {
    if (!initDone) return;
    bool presence;
    portENTER_CRITICAL(&overlayMux);
    presence = presenceAtomic;
    portEXIT_CRITICAL(&overlayMux);
    maybeSwitchPreset(presence);
  }

  void handleOverlayDraw() override {
    if (!initDone) return;
    int8_t segId = resolveTargetSegment();
    if (segId < 0) return;
    Segment &seg = strip.getSegment(segId);
    uint16_t segStart = seg.start;
    uint16_t segStop  = seg.stop;
    if (segStop <= segStart) return;
    uint16_t segLen   = segStop - segStart;
    uint64_t bits;
    portENTER_CRITICAL(&overlayMux);
    bits = overlayBits;
    portEXIT_CRITICAL(&overlayMux);

    if (segLen == HELMET_LEN) {
      uint16_t limit = (segLen < 64) ? segLen : 64;
      for (uint16_t i = 0; i < limit; i++) {
        if (bits & (1ULL << i)) {
          strip.setPixelColor(segStart + i, MARKER_COLOR);
        }
      }
    } else {
      for (uint16_t i = 0; i < HELMET_LEN && i < 64; i++) {
        if (bits & (1ULL << i)) {
          uint16_t mapped = (uint16_t)lroundf((float)i * (float)(segLen - 1) / (float)(HELMET_LEN - 1));
          strip.setPixelColor(segStart + mapped, MARKER_COLOR);
        }
      }
    }
  }

  void connected() override {}
  void addToJsonInfo(JsonObject&) override {}
  void addToJsonState(JsonObject&) override {}
  void readFromJsonState(JsonObject&) override {}
  void addToConfig(JsonObject&) override {}
  bool readFromConfig(JsonObject&) override { return true; }
  void appendConfigData() override {}
  bool handleButton(uint8_t) override { return false; }
#ifndef WLED_DISABLE_MQTT
  bool onMqttMessage(char*, char*) override { return false; }
  void onMqttConnect(bool) override {}
#endif
  void onStateChange(uint8_t) override {}

  uint16_t getId() override { return 151; }
};

static UsermodI2CDirectionV2 usermod_i2c_direction_v2;
REGISTER_USERMOD(usermod_i2c_direction_v2);
