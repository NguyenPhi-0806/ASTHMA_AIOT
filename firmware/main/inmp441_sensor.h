#ifndef INMP441_SENSOR_H
#define INMP441_SENSOR_H

#include <Arduino.h>

// ==== Ghi 1 doan co do dai co dinh de xuat .wav (dung rieng, khong lien quan ring buffer OLED) ====
#define INMP441_RECORD_TARGET_SAMPLES 48000 // 3 giay @ 16kHz

bool inmp441_startRecording();                          // bat dau gom mau, tu bat mic neu dang tat
bool inmp441_isRecording();                             // dang gom, chua du 48000 mau
bool inmp441_isRecordingReady();                        // da du 48000 mau, san sang lay ra gui
const int16_t *inmp441_getRecordingBuffer(int *outLen); // lay con tro + so luong mau
void inmp441_clearRecording();                          // reset sau khi da gui xong
// =========================
// AUDIO CONFIG
// =========================

#define INMP441_SAMPLE_RATE 16000

// Ring buffer 1 giay audio (int16 PCM), dung de xuat RAW cho data logger sau
// nay
#define INMP441_RING_BUFFER_SIZE 16000

bool inmp441_init();
void inmp441_update();

// Bat/tat viec do am thanh - chi bat khi co finger, de tranh
// thu am thanh moi truong luc khong do (du lieu training sach hon)
void inmp441_setActive(bool active);

// Monitoring (gia tri tinh tren window gan nhat)
float inmp441_getRMS();
int32_t inmp441_getPeak();

// Raw audio status
uint32_t inmp441_getSampleRate();
uint32_t inmp441_getTotalSamples();
float inmp441_getDurationSeconds();

// So sample/thuc te doc duoc trong 1 giay gan nhat (kiem tra rate ~ 16000)
uint32_t inmp441_getMeasuredRate();

// Timestamp (ms) cua lan doc sample gan nhat
unsigned long inmp441_getTimestampMs();

// Copy RAW PCM (int16) ra buffer ngoai - cho data logger / AI sau nay
// Tra ve so sample da copy (theo thu tu cu -> moi)
int inmp441_readRaw(int16_t *dest, int maxSamples);

#endif