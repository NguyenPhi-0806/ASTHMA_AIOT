#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>

bool oled_init();

void oled_showMessage(const char *title, const char *message);

void oled_showError(const char *message);

void oled_showReady();

void oled_update(float bpm, int averageBpm, long irValue, bool fingerDetected,

                 float rms, int32_t peak, uint32_t audioRate,

                 const int16_t *wave, int waveLen,

                 bool blynkConnected, unsigned long blynkRemainingSec,
                 bool blynkWindowValid);

#endif