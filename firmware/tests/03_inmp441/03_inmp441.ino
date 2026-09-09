// ============================================================
// INMP441 DIAGNOSTIC TEST
// Kiem tra mic co hoat dong on dinh khong:
//   - Do sample rate thuc te (mong doi ~16000 S/s)
//   - Phat hien loi "toan so 0" (sai chan L/R)
//   - Phat hien khong co du lieu (sai day / nguon)
//   - RMS/PEAK phan ung khi noi vao mic
//
// Luu y hieu nang: DMA 16x128 = 128 ms audio + I2C 400kHz +
// OLED chi ve moi 200ms de loop khong bi nghen (neu khong se
// mat sample va measured rate < 16000).
//
// Cach xem ket qua:
//   - Serial Monitor 115200: bao cao moi giay + PASS/FAIL
//   - Serial Plotter: xem song am (RMS)
//   - OLED: rate, RMS, PEAK, thanh muc do, verdict
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <math.h>

// =========================
// OLED
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define OLED_INTERVAL_MS 200

unsigned long lastOledMs = 0;

// =========================
// INMP441
// =========================

#define I2S_PORT I2S_NUM_0

#define I2S_SCK 14
#define I2S_WS 15
#define I2S_SD 16

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 256

// Nguong danh gia
#define RATE_MIN 14000
#define RATE_MAX 18000
#define PEAK_SILENCE 100  // PEAK duoi muc nay = gan nhu khong co tieng

int32_t samples[BUFFER_SIZE];

// Thong ke trong window 1 giay
unsigned long windowStartMs = 0;
uint32_t windowSamples = 0;
uint32_t measuredRate = 0;
uint32_t zeroSamples = 0;
float rmsValue = 0;
int32_t peakValue = 0;
bool allZero = false;

// =========================
// I2S INIT
// =========================

bool initI2S() {

  i2s_config_t config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,

      // 16 x 128 = 2048 samples ~ 128 ms audio
      .dma_buf_count = 16,
      .dma_buf_len = 128,

      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  if (i2s_driver_install(I2S_PORT, &config, 0, NULL) != ESP_OK) {
    return false;
  }

  if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK) {
    return false;
  }

  i2s_zero_dma_buffer(I2S_PORT);

  return true;
}

// =========================
// SETUP
// =========================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("============================");
  Serial.println("  INMP441 DIAGNOSTIC TEST");
  Serial.println("============================");
  Serial.println("SCK=GPIO14  WS=GPIO15  SD=GPIO16");
  Serial.println("L/R phai noi GND, VDD = 3.3V");
  Serial.println();

  Wire.begin(OLED_SDA, OLED_SCL);

  // 400kHz de khong nghet bus I2C lau
  Wire.setClock(400000);

  bool oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (oledOk) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("MIC TEST");
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.println("Init I2S...");
    display.display();
  } else {
    Serial.println("[WARN] OLED khong khoi dong duoc - chi xem Serial");
  }

  if (!initI2S()) {

    Serial.println("[FAIL] I2S driver install/set pin LOI!");
    Serial.println("=> Kiem tra lai firmware/board (ESP32-S3).");

    if (oledOk) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("I2S DRIVER FAIL");
      display.display();
    }

    while (1)
      ;
  }

  Serial.println("[OK] I2S driver installed, sampling 16 kHz");

  windowStartMs = millis();
}

// =========================
// REPORT MOI 1 GIAY
// =========================

void report() {

  unsigned long now = millis();

  if (now - windowStartMs < 1000) {
    return;
  }

  float elapsed = (now - windowStartMs) / 1000.0;

  measuredRate = (uint32_t)(windowSamples / elapsed);

  Serial.println();
  Serial.println("[AUDIO]");
  Serial.printf("measured   = %u S/s\n", measuredRate);
  Serial.printf("samples    = %u\n", windowSamples);
  Serial.printf("zeros      = %u\n", zeroSamples);
  Serial.printf("RMS        = %.0f\n", rmsValue);
  Serial.printf("PEAK       = %d\n", peakValue);

  // =========================
  // VERDICT
  // =========================

  const char *verdict;

  if (measuredRate == 0) {

    verdict = "KHONG CO DU LIEU - kiem tra day: SCK->14, WS->15, SD->16, VDD->3.3V, GND chung";

  } else if (allZero) {

    verdict = "DATA TOAN SO 0 - chan L/R cua INMP441 phai noi GND (hoac doi sang ONLY_RIGHT)";

  } else if (measuredRate < RATE_MIN || measuredRate > RATE_MAX) {

    verdict = "RATE LECH - loop bi nghen hoac day/ket noi khong on dinh";

  } else if (peakValue < PEAK_SILENCE) {

    verdict = "RATE OK nhung im lang - thu noi vao mic de xem PEAK tang";

  } else {

    verdict = "PASS - mic hoat dong on dinh, co tiep nhan am thanh";
  }

  Serial.printf("KET LUAN   = %s\n", verdict);

  // Reset window
  windowStartMs = now;
  windowSamples = 0;
  zeroSamples = 0;
}

// =========================
// LOOP
// =========================

void loop() {

  // =========================
  // DOC HET DU LIEU I2S CO SAN
  // =========================

  int64_t sum = 0;
  int32_t peak = 0;
  int readTotal = 0;
  int zeros = 0;

  for (int n = 0; n < 8; n++) {

    size_t bytesRead = 0;

    esp_err_t result =
        i2s_read(I2S_PORT, samples, sizeof(samples), &bytesRead, 0);

    if (result != ESP_OK || bytesRead == 0) {
      break;
    }

    int sampleCount = bytesRead / sizeof(int32_t);

    for (int i = 0; i < sampleCount; i++) {

      int32_t sample = samples[i] >> 14;

      if (sample == 0) {
        zeros++;
      }

      int32_t absolute = (sample < 0) ? -sample : sample;

      if (absolute > peak) {
        peak = absolute;
      }

      sum += (int64_t)sample * sample;
    }

    readTotal += sampleCount;
  }

  if (readTotal > 0) {
    rmsValue = sqrt((double)sum / readTotal);
    windowSamples += readTotal;
    zeroSamples += zeros;
  }

  peakValue = peak;

  allZero = (windowSamples > 0 && zeroSamples == windowSamples);

  // Serial Plotter: xem muc am thanh theo thoi gian
  Serial.println(rmsValue, 0);

  report();

  // =========================
  // OLED - chi ve moi 200ms
  // =========================

  if (millis() - lastOledMs < OLED_INTERVAL_MS) {
    return;
  }

  lastOledMs = millis();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("INMP441 TEST");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  display.setCursor(0, 13);
  display.print("RATE: ");
  if (measuredRate > 0) {
    display.print(measuredRate);
    display.println(" S/s");
  } else {
    display.println("...");
  }

  display.setCursor(0, 24);
  display.print("RMS : ");
  display.println(rmsValue, 0);

  display.setCursor(0, 35);
  display.print("PEAK: ");
  display.println(peakValue);

  // Thanh muc do theo PEAK
  int barWidth = map(constrain(peakValue, 0, 10000), 0, 10000, 0, 124);
  display.drawRect(0, 46, 128, 10, SSD1306_WHITE);
  display.fillRect(2, 48, barWidth, 6, SSD1306_WHITE);

  // Trang thai ngan gon
  display.setCursor(0, 58);
  if (measuredRate == 0) {
    display.print("NO DATA");
  } else if (allZero) {
    display.print("ALL ZERO (L/R?)");
  } else if (measuredRate >= RATE_MIN && measuredRate <= RATE_MAX &&
             peakValue >= PEAK_SILENCE) {
    display.print("PASS");
  } else {
    display.print("CHECK...");
  }

  display.display();
}
