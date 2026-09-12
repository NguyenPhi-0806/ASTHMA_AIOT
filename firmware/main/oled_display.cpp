#include "oled_display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// =========================
// OLED
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 8
#define OLED_SCL 9

#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =========================
// INIT
// =========================

bool oled_init() {

  Serial.println();
  Serial.println("[OLED] Initializing...");

  Wire.begin(OLED_SDA, OLED_SCL);

  // 400kHz de lan ve OLED (~1KB) khong chiem bus I2C qua lau
  // (100kHz mac dinh lam nghet bus, lam mat sample cua INMP441)
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {

    Serial.println("[OLED] ERROR!");

    return false;
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.display();

  Serial.println("[OLED] OK");

  return true;
}

// =========================
// MESSAGE
// =========================

void oled_showMessage(const char *title, const char *message) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);

  display.setCursor(0, 0);

  display.println(title);

  display.setTextSize(1);

  display.setCursor(0, 30);

  display.println(message);

  display.display();
}

// =========================
// ERROR
// =========================

void oled_showError(const char *message) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("SYSTEM ERROR");

  display.println();

  display.println(message);

  display.display();
}

// =========================
// READY
// =========================

void oled_showReady() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);

  display.setCursor(0, 0);

  display.println("READY");

  display.setTextSize(1);

  display.setCursor(0, 28);

  display.println("MAX30102: OK");

  display.setCursor(0, 40);

  display.println("INMP441 : 16kHz OK");

  display.setCursor(0, 52);

  display.println("System running");

  display.display();
}

// =========================
// WAVEFORM
// =========================

static void drawWaveform(const int16_t *wave, int waveLen) {

  const int midY = 31; // duong giua vung song
  const int halfH = 8; // bien do toi da 2 ben

  if (wave == NULL || waveLen <= 0) {
    return;
  }

  // Auto-scale: tim bien do lon nhat trong buffer,
  // de muc san (300) de tieng nho van nhin thay song
  int32_t maxAmp = 300;

  for (int i = 0; i < waveLen; i++) {

    int32_t a = (wave[i] < 0) ? -wave[i] : wave[i];

    if (a > maxAmp) {
      maxAmp = a;
    }
  }

  // Duong co so
  display.drawFastHLine(0, midY, SCREEN_WIDTH, SSD1306_WHITE);

  // Moi cot 1 diem mau cua man hinh = 1 sample
  for (int x = 0; x < SCREEN_WIDTH && x < waveLen; x++) {

    int32_t v = ((int32_t)wave[x] * halfH) / maxAmp;

    if (v > halfH) {
      v = halfH;
    }

    if (v < -halfH) {
      v = -halfH;
    }

    int y = midY - v;

    if (y == midY) {
      display.drawPixel(x, midY, SSD1306_WHITE);
    } else {
      display.drawFastVLine(x, (y < midY ? y : midY), abs(v) + 1,
                            SSD1306_WHITE);
    }
  }
}

// =========================
// UPDATE
// =========================

void oled_update(float bpm, int averageBpm, long irValue, bool fingerDetected,

                 float rms, int32_t peak, uint32_t audioRate,

                 const int16_t *wave, int waveLen,

                 bool wifiConnected,

                 bool blynkConnected, unsigned long blynkRemainingSec,
                 bool blynkWindowValid) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  // =========================
  // TITLE
  // =========================

  display.setCursor(0, 0);

  display.println("ASTHMA AIoT");

  display.setCursor(86, 0);

  display.print("W:");

  display.print(wifiConnected ? "OK" : "--");

  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // =========================
  // MAX30102 (1 dong)
  // =========================

  display.setCursor(0, 12);

  display.print("HR:");

  if (fingerDetected && averageBpm > 0) {

    display.print(averageBpm);

  } else {

    display.print("--");
  }

  display.print(" IR:");

  display.println(irValue);

  // =========================
  // WAVEFORM (y 22..40)
  // =========================

  drawWaveform(wave, waveLen);

  // =========================
  // INMP441
  // =========================

  display.setCursor(0, 42);

  display.print("RMS:");

  display.print(rms, 0);

  display.print(" PK:");

  display.print(peak);

  display.setCursor(0, 51);

  display.print("MIC: ");

  if (audioRate == 0) {

    display.print("NO DATA");

  } else {

    display.print(audioRate);

    display.print(" S/s");

    if (audioRate > 14000 && audioRate < 18000) {
      display.print(" OK");
    }
  }

  // =========================
  // STATUS (trai: finger, phai: Blynk)
  // =========================

  display.setCursor(0, 59);

  if (fingerDetected) {

    display.print("FINGER");

  } else {

    display.print("NO FINGER");
  }

  display.setCursor(70, 59);

  if (!blynkConnected) {

    // Chua ket noi duoc Blynk cloud
    display.print("B:--");

  } else if (!blynkWindowValid) {

    // Da mat finger it nhat 1 lan trong cua so nay -> phut nay se gui 0
    display.print("B:X");

  } else {

    // Van dang hop le - dem nguoc con bao nhieu giay nua thi push
    display.print("B:");

    display.print(blynkRemainingSec);

    display.print("s");
  }

  display.display();
}