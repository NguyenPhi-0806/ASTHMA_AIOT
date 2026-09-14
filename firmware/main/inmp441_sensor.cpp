#include "inmp441_sensor.h"

#include <driver/i2s.h>
#include <math.h>

// =========================
// I2S
// =========================

#define I2S_PORT I2S_NUM_0

#define I2S_SCK 14
#define I2S_WS 15
#define I2S_SD 16

// =========================
// AUDIO
// =========================

#define SAMPLE_RATE INMP441_SAMPLE_RATE
#define BUFFER_SIZE 256

// Report Serial moi 1 giay de kiem tra sample rate
#define STATS_INTERVAL_MS 1000

int32_t rawSamples[BUFFER_SIZE];

// Ring buffer RAW PCM int16 - du lieu dai dien cho AI training sau nay
static int16_t ringBuffer[INMP441_RING_BUFFER_SIZE];
static int16_t recordBuffer[INMP441_RECORD_TARGET_SAMPLES];
static uint32_t recordIndex = 0;
static bool recording = false;
static bool recordingReady = false;
static uint32_t ringWriteIndex = 0;

static uint32_t totalSamples = 0;
static uint32_t windowSamples = 0;
static uint32_t measuredRate = 0;

static unsigned long lastStatsMs = 0;
static unsigned long lastTimestampMs = 0;

static float rmsValue = 0;
static int32_t peakValue = 0;

// Mac dinh tat - chi bat khi main.ino goi inmp441_setActive(true) luc co finger
static bool audioActive = false;

// =========================
// INIT
// =========================

bool inmp441_init()
{

  Serial.println();
  Serial.println("[INMP441] Initializing...");

  i2s_config_t config = {

      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),

      .sample_rate = SAMPLE_RATE,

      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,

      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,

      .communication_format = I2S_COMM_FORMAT_I2S,

      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,

      // 16 x 128 = 2048 samples ~ 128 ms audio:
      // du cho ca nhung luc bus I2C (OLED/MAX30102) dang ban
      .dma_buf_count = 16,

      .dma_buf_len = 128,

      .use_apll = false,

      .tx_desc_auto_clear = false,

      .fixed_mclk = 0

  };

  i2s_pin_config_t pin_config = {

      .bck_io_num = I2S_SCK,

      .ws_io_num = I2S_WS,

      .data_out_num = I2S_PIN_NO_CHANGE,

      .data_in_num = I2S_SD

  };

  esp_err_t result;

  result = i2s_driver_install(I2S_PORT, &config, 0, NULL);

  if (result != ESP_OK)
  {

    Serial.println("[INMP441] Driver install failed!");

    return false;
  }

  result = i2s_set_pin(I2S_PORT, &pin_config);

  if (result != ESP_OK)
  {

    Serial.println("[INMP441] Pin configuration failed!");

    return false;
  }

  i2s_zero_dma_buffer(I2S_PORT);

  // Mac dinh tat mic ngay tu dau - chi bat khi main.ino phat hien co finger
  i2s_stop(I2S_PORT);

  memset(ringBuffer, 0, sizeof(ringBuffer));
  ringWriteIndex = 0;
  totalSamples = 0;
  windowSamples = 0;
  measuredRate = 0;
  lastStatsMs = millis();
  lastTimestampMs = 0;

  Serial.println("[INMP441] OK");
  Serial.println("[INMP441] Sample rate: 16 kHz");
  Serial.println("[INMP441] RAW PCM: int16, ring buffer 1 sec");

  return true;
}

// =========================
// UPDATE
// =========================

void inmp441_update()
{

  // Khong co finger -> khong doc mic, tranh lay tap am moi truong
  // lan vao du lieu (giu du lieu training sach: chi co am thanh
  // luc thuc su dang do benh nhan)
  if (!audioActive)
  {
    return;
  }

  int64_t sum = 0;

  int32_t peak = 0;

  int readCount = 0;

  // Rut het du lieu co san trong DMA moi vong loop.
  // Mot lan doc chi lay toi da 256 sample, neu loop chay cham hon
  // toc do audio thi doc 1 lan se tran DMA -> mat sample.
  for (int n = 0; n < 8; n++)
  {

    size_t bytesRead = 0;

    esp_err_t result =
        i2s_read(I2S_PORT, rawSamples, sizeof(rawSamples), &bytesRead, 0);

    if (result != ESP_OK || bytesRead == 0)
    {

      break;
    }

    int sampleCount = bytesRead / sizeof(int32_t);

    for (int i = 0; i < sampleCount; i++)
    {
      int16_t sample = (int16_t)(rawSamples[i] >> 16);

      // ---- THEM: gom mau cho file .wav dang ghi (neu co) ----
      if (recording && recordIndex < INMP441_RECORD_TARGET_SAMPLES)
      {
        recordBuffer[recordIndex++] = sample;
        if (recordIndex >= INMP441_RECORD_TARGET_SAMPLES)
        {
          recording = false;
          recordingReady = true;
        }
      }
      // ---- HET PHAN THEM ----

      // ---- THEM: gom mau vao ring buffer ----
      int32_t absolute = (sample < 0) ? -sample : sample;

      if (absolute > peak)
      {
        peak = absolute;
      }

      sum += (int64_t)sample * sample;
    }

    readCount += sampleCount;
  }

  if (readCount > 0)
  {

    rmsValue = sqrt((double)sum / readCount);

    totalSamples += readCount;

    windowSamples += readCount;

    lastTimestampMs = millis();
  }

  peakValue = peak;

  // =========================
  // SERIAL REPORT (1 sec)
  // =========================

  unsigned long now = millis();

  if (now - lastStatsMs >= STATS_INTERVAL_MS)
  {

    float elapsed = (now - lastStatsMs) / 1000.0;

    if (elapsed > 0)
    {
      measuredRate = (uint32_t)(windowSamples / elapsed);
    }

    Serial.println();
    Serial.println("[AUDIO]");
    Serial.printf("rate       = %u Hz\n", SAMPLE_RATE);
    Serial.printf("measured   = %u S/s\n", measuredRate);
    Serial.printf("samples    = %u\n", windowSamples);
    Serial.printf("total      = %u\n", totalSamples);
    Serial.printf("duration   = %.3f sec\n", elapsed);
    Serial.printf("RMS        = %.0f\n", rmsValue);
    Serial.printf("PEAK       = %d\n", peakValue);

    lastStatsMs = now;
    windowSamples = 0;
  }
}

// =========================
// GETTERS
// =========================

float inmp441_getRMS() { return rmsValue; }

int32_t inmp441_getPeak() { return peakValue; }

uint32_t inmp441_getSampleRate() { return SAMPLE_RATE; }

uint32_t inmp441_getTotalSamples() { return totalSamples; }

float inmp441_getDurationSeconds() { return (float)totalSamples / SAMPLE_RATE; }

uint32_t inmp441_getMeasuredRate() { return measuredRate; }

unsigned long inmp441_getTimestampMs() { return lastTimestampMs; }

void inmp441_setActive(bool active)
{

  if (active == audioActive)
  {
    return;
  }

  audioActive = active;

  if (!active)
  {

    // Tat mic that su (dung xung clock I2S) - khong chi ngung doc,
    // ma dung luon phan cung de tiet kiem dien va khong con tin
    // hieu nao duoc tao ra
    i2s_stop(I2S_PORT);

    // Xoa cac gia tri hien thi/gui len Blynk ve 0 - khong con la
    // "gia tri cu" cua lan do truoc
    rmsValue = 0;
    peakValue = 0;
    measuredRate = 0;
    windowSamples = 0;
  }
  else
  {

    // Vua bat lai mic - xoa sach DMA buffer cu (im lang luc chua
    // bat) truoc khi bat dau doc, tranh lay nham khoang lang do
    i2s_zero_dma_buffer(I2S_PORT);

    i2s_start(I2S_PORT);

    lastStatsMs = millis();
  }
}

int inmp441_readRaw(int16_t *dest, int maxSamples)
{

  if (dest == NULL || maxSamples <= 0)
  {
    return 0;
  }

  int available = INMP441_RING_BUFFER_SIZE;

  if ((uint32_t)available > totalSamples)
  {
    available = totalSamples;
  }

  if (available > maxSamples)
  {
    available = maxSamples;
  }

  // Vitri sample cu nhat con lai trong ring buffer
  uint32_t start = (ringWriteIndex + INMP441_RING_BUFFER_SIZE - available) %
                   INMP441_RING_BUFFER_SIZE;

  for (int i = 0; i < available; i++)
  {
    dest[i] = ringBuffer[(start + i) % INMP441_RING_BUFFER_SIZE];
  }

  return available;
}
bool inmp441_startRecording()
{
  if (recording)
    return false; // dang ghi do dang khac, bo qua
  recordIndex = 0;
  recordingReady = false;
  recording = true;
  if (!audioActive)
  {
    inmp441_setActive(true); // ep bat mic du khong co finger
  }
  return true;
}

bool inmp441_isRecording() { return recording; }
bool inmp441_isRecordingReady() { return recordingReady; }

const int16_t *inmp441_getRecordingBuffer(int *outLen)
{
  if (outLen)
    *outLen = recordingReady ? INMP441_RECORD_TARGET_SAMPLES : 0;
  return recordBuffer;
}

void inmp441_clearRecording()
{
  recordingReady = false;
  recordIndex = 0;
}