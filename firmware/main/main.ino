#include <Arduino.h>
//
#include "blynk_manager.h"
#include "inmp441_sensor.h"
#include "max30102_sensor.h"
#include "oled_display.h"
#include "wifi_manager.h"

unsigned long lastDisplayUpdate = 0;

const unsigned long DISPLAY_INTERVAL = 200;

// Buffer waveform cho OLED (128 cot = rong man hinh)
static int16_t waveBuf[128];

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("      ASTHMA AIoT");
  Serial.println("      MAIN FIRMWARE");
  Serial.println("==============================");

  // ==========================
  // OLED
  // ==========================

  Serial.println("[SYSTEM] Starting OLED...");

  if (!oled_init()) {
    Serial.println("[ERROR] OLED initialization failed!");

    while (true) {
      delay(1000);
    }
  }

  oled_showMessage("ASTHMA AIoT", "Initializing...");

  delay(500);

  // ==========================
  // MAX30102
  // ==========================

  Serial.println("[SYSTEM] Starting MAX30102...");

  if (!max30102_init()) {
    Serial.println("[ERROR] MAX30102 initialization failed!");

    oled_showError("MAX30102 ERROR");

    while (true) {
      delay(1000);
    }
  }

  delay(500);

  // ==========================
  // INMP441
  // ==========================

  Serial.println("[SYSTEM] Starting INMP441...");

  if (!inmp441_init()) {
    Serial.println("[ERROR] INMP441 initialization failed!");

    oled_showError("INMP441 ERROR");

    while (true) {
      delay(1000);
    }
  }

  delay(500);

  // ==========================
  // WIFI
  // ==========================

  Serial.println("[SYSTEM] Starting WiFi...");

  bool wifiOk = wifi_init();

  // ==========================
  // BLYNK
  // ==========================

  blynk_init();

  // ==========================
  // SYSTEM READY
  // ==========================

  Serial.println();
  Serial.println("==============================");
  Serial.println("      SYSTEM READY");
  Serial.printf("      WiFi : %s\n", wifiOk ? "CONNECTED" : "OFFLINE");
  Serial.println("==============================");

  oled_showReady();

  delay(1500);
}

void loop() {
  // ==========================
  // READ MAX30102
  // ==========================

  max30102_update();

  // ==========================
  // READ INMP441
  // ==========================

  inmp441_update();

  // ==========================
  // WIFI + BLYNK
  // ==========================

  wifi_update();

  blynk_setHealth(max30102_getAverageBPM(), max30102_hasFinger(),
                  inmp441_getRMS(), inmp441_getMeasuredRate());

  blynk_update();

  // ==========================
  // UPDATE OLED
  // ==========================

  if (millis() - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = millis();

    int waveLen = inmp441_readRaw(waveBuf, 128);

    oled_update(max30102_getBPM(), max30102_getAverageBPM(), max30102_getIR(),
                max30102_hasFinger(),

                inmp441_getRMS(), inmp441_getPeak(), inmp441_getMeasuredRate(),
                waveBuf, waveLen,

                blynk_isConnected(), blynk_getWindowRemainingSec(),
                blynk_isWindowValidSoFar());
  }
}
