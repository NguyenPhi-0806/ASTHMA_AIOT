#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// Ket noi WiFi (block toi da 15s). Tra ve true neu ket noi duoc.
// Neu that bai, he thong van tiep tuc chay offline.
bool wifi_init();

// Goi trong loop - tu reconnect khi mat ket noi
void wifi_update();

bool wifi_isConnected();

int32_t wifi_getRSSI();

String wifi_getIP();

#endif
