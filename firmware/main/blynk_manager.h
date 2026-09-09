#ifndef BLYNK_MANAGER_H
#define BLYNK_MANAGER_H

#include <Arduino.h>

// Config Blynk (khong block - ket noi dien ra trong blynk_update)
bool blynk_init();

// Goi trong loop - chay Blynk.run() va tu dong push data moi 1 phut
// (chi hop le neu finger duoc giu lien tuc suot ca phut do)
void blynk_update();

bool blynk_isConnected();

// So giay con lai truoc khi het cua so 1 phut hien tai (0 neu vua push xong)
unsigned long blynk_getWindowRemainingSec();

// Cua so hien tai co con hop le khong (finger chua bi rut ra lan nao
// tu dau cua so den gio) - dung de hien thi len OLED
bool blynk_isWindowValidSoFar();

// Cap nhat gia tri moi nhat tu sensor - se duoc push trong blynk_update
void blynk_setHealth(int avgBpm, bool fingerDetected, float rms,
                     uint32_t audioRate);

#endif