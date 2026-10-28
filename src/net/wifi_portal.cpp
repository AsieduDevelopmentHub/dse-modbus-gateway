#if defined(ARDUINO_ARCH_ESP32)

#include "net/wifi_portal.h"

#include <WiFi.h>
#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "telemetry/snapshot.h"

namespace {

char gMode[12] = "off";
char gIp[16] = "0.0.0.0";
char gApSsid[32] = {};

void buildApSsid() {
  uint64_t mac = ESP.getEfuseMac();
  const uint16_t suffix =
      static_cast<uint16_t>((mac >> 32) ^ (mac & 0xFFFFu)) & 0xFFFFu;
  snprintf(gApSsid, sizeof(gApSsid), "%s-%04X", WIFI_AP_SSID_PREFIX, suffix);
}

void refreshInfo() {
  if (WiFi.getMode() == WIFI_AP) {
    strncpy(gMode, "AP", sizeof(gMode) - 1);
    strncpy(gIp, WiFi.softAPIP().toString().c_str(), sizeof(gIp) - 1);
    Telemetry::setWifiInfo(gMode, gIp, 0);
    return;
  }
  if (WiFi.getMode() == WIFI_AP_STA) {
    if (WiFi.status() == WL_CONNECTED) {
      strncpy(gMode, "AP+STA", sizeof(gMode) - 1);
      strncpy(gIp, WiFi.localIP().toString().c_str(), sizeof(gIp) - 1);
      Telemetry::setWifiInfo(gMode, gIp, static_cast<int8_t>(WiFi.RSSI()));
    } else {
      strncpy(gMode, "AP", sizeof(gMode) - 1);
      strncpy(gIp, WiFi.softAPIP().toString().c_str(), sizeof(gIp) - 1);
      Telemetry::setWifiInfo(gMode, gIp, 0);
    }
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    strncpy(gMode, "STA", sizeof(gMode) - 1);
    strncpy(gIp, WiFi.localIP().toString().c_str(), sizeof(gIp) - 1);
    Telemetry::setWifiInfo(gMode, gIp, static_cast<int8_t>(WiFi.RSSI()));
    return;
  }
  strncpy(gMode, "off", sizeof(gMode) - 1);
  strncpy(gIp, "0.0.0.0", sizeof(gIp) - 1);
  Telemetry::setWifiInfo(gMode, gIp, 0);
}

}  // namespace

namespace WifiPortal {

void begin() {
  buildApSsid();
  const bool wantSta =
      WIFI_STA_SSID[0] != '\0' && strlen(WIFI_STA_SSID) > 0;

  WiFi.persistent(false);
  WiFi.setSleep(false);
  if (wantSta) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
  } else {
    WiFi.mode(WIFI_AP);
  }

  // AP password must be >= 8 chars for WPA2; open AP if too short.
  const bool openAp = strlen(WIFI_AP_PASSWORD) < 8;
  if (openAp) {
    WiFi.softAP(gApSsid);
  } else {
    WiFi.softAP(gApSsid, WIFI_AP_PASSWORD);
  }

  Serial.println(F("[WiFi] Soft-AP started"));
  Serial.print(F("[WiFi] SSID: "));
  Serial.println(gApSsid);
  if (!openAp) {
    Serial.print(F("[WiFi] Pass: "));
    Serial.println(WIFI_AP_PASSWORD);
  }
  Serial.print(F("[WiFi] AP IP: "));
  Serial.println(WiFi.softAPIP());
  if (wantSta) {
    Serial.print(F("[WiFi] Joining STA: "));
    Serial.println(WIFI_STA_SSID);
  }

  refreshInfo();
}

void poll() { refreshInfo(); }

const char *mode() { return gMode; }
const char *ip() { return gIp; }
int8_t rssi() {
  if (WiFi.status() == WL_CONNECTED) {
    return static_cast<int8_t>(WiFi.RSSI());
  }
  return 0;
}

}  // namespace WifiPortal

#endif  // ARDUINO_ARCH_ESP32
