#include "max30102_sensor.h"

#include "MAX30105.h"
#include "heartRate.h"
#include <Wire.h>

// =========================
// PIN
// =========================

#define MAX30102_SDA 8
#define MAX30102_SCL 9

// =========================
// CONFIG
// =========================

#define FINGER_THRESHOLD 50000

const byte RATE_SIZE = 4;

// =========================
// SENSOR
// =========================

MAX30105 particleSensor;

// =========================
// HEART RATE
// =========================

byte rates[RATE_SIZE];

byte rateSpot = 0;

long lastBeat = 0;

float beatsPerMinute = 0;

int beatAvg = 0;

// Cache gia tri IR cua lan doc gan nhat - tranh doc I2C nhieu lan moi loop
// (bus I2C bi chiem se lam INMP441 mat sample do DMA tran)
static long lastIRValue = 0;

// =========================
// INIT
// =========================

bool max30102_init() {

  Serial.println();
  Serial.println("[MAX30102] Initializing...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {

    Serial.println("[MAX30102] Sensor not found!");

    return false;
  }

  particleSensor.setup();

  particleSensor.setPulseAmplitudeRed(0x1F);

  particleSensor.setPulseAmplitudeIR(0x1F);

  particleSensor.setPulseAmplitudeGreen(0);

  Serial.println("[MAX30102] OK");

  return true;
}

// =========================
// UPDATE
// =========================

void max30102_update() {

  lastIRValue = particleSensor.getIR();

  long irValue = lastIRValue;

  // =========================
  // NO FINGER
  // =========================

  if (irValue < FINGER_THRESHOLD) {

    beatsPerMinute = 0;
    beatAvg = 0;

    return;
  }

  // =========================
  // BEAT DETECTION
  // =========================

  if (checkForBeat(irValue)) {

    long delta = millis() - lastBeat;

    lastBeat = millis();

    beatsPerMinute = 60.0 / (delta / 1000.0);

    if (beatsPerMinute > 20 && beatsPerMinute < 255) {

      rates[rateSpot++] = (byte)beatsPerMinute;

      rateSpot %= RATE_SIZE;

      beatAvg = 0;

      for (byte x = 0; x < RATE_SIZE; x++) {

        beatAvg += rates[x];
      }

      beatAvg /= RATE_SIZE;
    }
  }
}

// =========================
// GETTERS
// =========================

long max30102_getIR() { return lastIRValue; }

float max30102_getBPM() { return beatsPerMinute; }

int max30102_getAverageBPM() { return beatAvg; }

bool max30102_hasFinger() { return lastIRValue >= FINGER_THRESHOLD; }
