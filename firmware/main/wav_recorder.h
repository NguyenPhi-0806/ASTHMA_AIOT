/*
 * wav_recorder.h
 * ASTHMA_AIOT - Dieu khien ghi 1 doan 3 giay va gui qua WiFi de dong goi .wav
 *
 * KHONG tu cai I2S - dung lai du lieu tu inmp441_sensor.cpp (ham
 * inmp441_startRecording() / inmp441_getRecordingBuffer() ...) de tranh
 * xung dot voi driver I2S da duoc inmp441_init() cai san.
 */

#ifndef WAV_RECORDER_H
#define WAV_RECORDER_H

#include <Arduino.h>

// Goi 1 lan trong setup()
void wavRecorderInit();

// Cau hinh IP/port cua may tinh chay pc_wav_receiver.py
void wavRecorderSetServer(const char *ip, uint16_t port);

// Goi trong loop() voi 1 dong lenh: "normal" | "cough" | "wheeze" | "record"
void wavRecorderHandleCommand(const String &cmd);

// Goi trong loop() MOI VONG LAP - kiem tra xem doan ghi da du 48000 mau
// chua, neu du thi tu dong gui qua TCP va reset. Khong blocking.
void wavRecorderUpdate();

#endif // WAV_RECORDER_H