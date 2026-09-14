/*
 * wav_recorder.cpp
 * Dung lai du lieu tu inmp441_sensor (khong tu cai I2S) de tranh xung dot
 * voi driver da duoc inmp441_init() cai san tren I2S_NUM_0.
 */

#include "wav_recorder.h"
#include "inmp441_sensor.h"
#include <WiFi.h>

static String s_currentLabel = "normal";
static int s_fileIndex = 1;
static bool s_waitingToSend = false;

static const char *s_serverIp = "192.168.59.206";
static uint16_t s_serverPort = 5005;

static bool wavSendToServer(const int16_t *buf, int sampleCount,
                            const String &label, int index)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[wav_recorder] LOI: WiFi chua ket noi, bo qua gui file.");
    return false;
  }

  WiFiClient client;
  if (!client.connect(s_serverIp, s_serverPort))
  {
    Serial.println("[wav_recorder] LOI: khong ket noi duoc server nhan WAV.");
    return false;
  }

  unsigned long totalBytes = (unsigned long)sampleCount * sizeof(int16_t);

  char header[64];
  snprintf(header, sizeof(header), "%s|%03d|%lu\n", label.c_str(), index, totalBytes);
  client.print(header);
  client.write((const uint8_t *)buf, totalBytes);
  client.flush();
  client.stop();
  return true;
}

void wavRecorderSetServer(const char *ip, uint16_t port)
{
  s_serverIp = ip;
  s_serverPort = port;
}

void wavRecorderInit()
{
  Serial.println("[wav_recorder] San sang. Lenh: normal | cough | wheeze | record");
}

void wavRecorderHandleCommand(const String &cmd)
{
  if (cmd == "normal" || cmd == "cough" || cmd == "wheeze")
  {
    s_currentLabel = cmd;
    s_fileIndex = 1;
    Serial.println("[wav_recorder] Da doi nhan: " + s_currentLabel);
    return;
  }

  if (cmd == "record")
  {
    if (inmp441_isRecording() || s_waitingToSend)
    {
      Serial.println("[wav_recorder] Dang ghi/gui doan truoc, cho xong da.");
      return;
    }
    if (inmp441_startRecording())
    {
      s_waitingToSend = true;
      Serial.println("[wav_recorder] Bat dau ghi 3 giay...");
    }
    else
    {
      Serial.println("[wav_recorder] Khong bat dau ghi duoc.");
    }
    return;
  }

  Serial.println("[wav_recorder] Lenh khong hop le: normal | cough | wheeze | record");
}

void wavRecorderUpdate()
{
  if (!s_waitingToSend)
    return;
  if (!inmp441_isRecordingReady())
    return; // chua du 48000 mau, cho tiep

  int len = 0;
  const int16_t *buf = inmp441_getRecordingBuffer(&len);

  Serial.println("[wav_recorder] Ghi xong, dang gui...");
  if (wavSendToServer(buf, len, s_currentLabel, s_fileIndex))
  {
    Serial.printf("[wav_recorder] Da gui %s_%03d.wav (%d mau)\n",
                  s_currentLabel.c_str(), s_fileIndex, len);
    s_fileIndex++;
  }

  inmp441_clearRecording();
  s_waitingToSend = false;
}