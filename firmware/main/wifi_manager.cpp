#include "wifi_manager.h"

#include "secrets.h"
#include <WiFi.h>

// Thu lai moi 10 giay khi mat ket noi
#define RECONNECT_INTERVAL_MS 10000

#define CONNECT_TIMEOUT_MS 15000

static unsigned long lastReconnectMs = 0;

// =========================
// INIT
// =========================

bool wifi_init() {

  Serial.println();
  Serial.println("[WIFI] Connecting to: " + String(WIFI_SSID));

  WiFi.mode(WIFI_STA);

  WiFi.setHostname("ASTHMA-AIOT");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < CONNECT_TIMEOUT_MS) {

    delay(250);

    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.printf("[WIFI] OK - IP: %s, RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());

    return true;
  }

  Serial.println("[WIFI] Khong ket noi duoc - he thong van chay offline");

  return false;
}

// =========================
// UPDATE
// =========================

void wifi_update() {

  if (WiFi.status() == WL_CONNECTED) {

    lastReconnectMs = millis();

    return;
  }

  if (millis() - lastReconnectMs < RECONNECT_INTERVAL_MS) {

    return;
  }

  lastReconnectMs = millis();

  Serial.println("[WIFI] Mat ket noi - reconnecting...");

  WiFi.disconnect();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// =========================
// GETTERS
// =========================

bool wifi_isConnected() { return WiFi.status() == WL_CONNECTED; }

int32_t wifi_getRSSI() { return WiFi.RSSI(); }

String wifi_getIP() { return WiFi.localIP().toString(); }
