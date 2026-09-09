// Luu y: secrets.h PHAI duoc include truoc BlynkSimpleEsp32.h
// vi BLYNK_TEMPLATE_ID / BLYNK_AUTH_TOKEN phai dinh nghia san

#define BLYNK_PRINT Serial

#include "secrets.h"

#include <BlynkSimpleEsp32.h>

#include "blynk_manager.h"

#include "wifi_manager.h"

// Push data moi 1 phut - va chi tinh la hop le neu finger duoc giu
// LIEN TUC suot ca cua so 1 phut do (rut tay ra giua chung la coi nhu
// cua so do khong hop le, gui 0 / NO FINGER)
#define SEND_INTERVAL_MS 60000

// Virtual pins (tao trong Blynk Console -> Datastreams)
// V0 = Heart Rate (BPM)
// V1 = SpO2 (chua lam - buoc 2 trong ke hoach)
// V2 = Audio RMS
// V3 = Audio Status (chuoi)

struct {

  int avgBpm;

  bool finger;

  float rms;

  uint32_t audioRate;

} blynkCache = {0, false, 0, 0};

static unsigned long windowStartMs = 0;

// true suot tu dau cua so den gio - se bi ha xuong false ngay khi
// phat hien mat finger du chi 1 lan trong cua so 1 phut
static bool fingerHeldWholeWindow = true;

// =========================
// INIT
// =========================

bool blynk_init() {

  Serial.println("[BLYNK] Configuring...");

  Blynk.config(BLYNK_AUTH_TOKEN);

  // Khong goi Blynk.connect() o day de tranh block.
  // Blynk.run() trong blynk_update() se tu ket noi khi co WiFi.
  Serial.println("[BLYNK] OK (se ket noi trong loop)");

  return true;
}

// =========================
// SET DATA
// =========================

void blynk_setHealth(int avgBpm, bool fingerDetected, float rms,
                     uint32_t audioRate) {

  blynkCache.avgBpm = avgBpm;

  blynkCache.finger = fingerDetected;

  blynkCache.rms = rms;

  blynkCache.audioRate = audioRate;
}

// =========================
// UPDATE
// =========================

void blynk_update() {

  if (!wifi_isConnected()) {

    return;
  }

  Blynk.run();

  if (!Blynk.connected()) {

    return;
  }

  // Theo doi lien tuc trong suot cua so: chi can 1 lan khong co finger
  // la coi ca cua so 1 phut nay khong hop le
  if (!blynkCache.finger) {

    fingerHeldWholeWindow = false;
  }

  if (millis() - windowStartMs < SEND_INTERVAL_MS) {

    return;
  }

  // Het 1 phut - chot ket qua cua so nay lai

  // Hop le khi: finger duoc giu suot ca cua so VA hien tai van dang co finger
  bool windowValid = fingerHeldWholeWindow && blynkCache.finger;

  int bpm = (windowValid && blynkCache.avgBpm > 0) ? blynkCache.avgBpm : 0;

  Blynk.virtualWrite(V0, bpm);

  if (windowValid) {

    // Co finger du 1 phut -> audio moi duoc tinh la du lieu that
    Blynk.virtualWrite(V2, (int)blynkCache.rms);

    bool audioOk =
        (blynkCache.audioRate > 14000 && blynkCache.audioRate < 18000);

    Blynk.virtualWrite(V3, audioOk ? "OK" : "NO DATA");

  } else {

    // Khong giu finger du 1 phut -> khong tinh la co thu am hop le
    Blynk.virtualWrite(V2, 0);

    Blynk.virtualWrite(V3, "NO FINGER");
  }

  // Reset lai cho cua so 1 phut tiep theo
  windowStartMs = millis();

  fingerHeldWholeWindow = true;
}

// =========================
// GETTERS
// =========================

bool blynk_isConnected() { return Blynk.connected(); }

unsigned long blynk_getWindowRemainingSec() {

  unsigned long elapsed = millis() - windowStartMs;

  if (elapsed >= SEND_INTERVAL_MS) {
    return 0;
  }

  return (SEND_INTERVAL_MS - elapsed) / 1000;
}

bool blynk_isWindowValidSoFar() { return fingerHeldWholeWindow; }