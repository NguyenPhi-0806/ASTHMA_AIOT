#ifndef INMP441_SENSOR_H
#define INMP441_SENSOR_H

#include <Arduino.h>

// =========================
// AUDIO CONFIG
// =========================

#define INMP441_SAMPLE_RATE 16000

// Ring buffer 1 giay audio (int16 PCM), dung de xuat RAW cho data logger sau nay
#define INMP441_RING_BUFFER_SIZE 16000

bool inmp441_init();
void inmp441_update();

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
