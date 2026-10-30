#if defined(ARDUINO_ARCH_ESP32)

#include "net/wifi_portal.h"

#include <WiFi.h>
#include <stdio.h>
#include <string.h>

#include <esp_system.h>

#include "config/config.h"
#include "config/settings.h"
#include "telemetry/snapshot.h"
#include "util/str_util.h"

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

void copyText(char *dst, size_t n, const char *src) {
  if (!dst || n == 0) return;
  strncpy(dst, src ? src : "", n - 1);
  dst[n - 1] = '\0';
}

void refreshInfo() {
  const wifi_mode_t radio = WiFi.getMode();
  int8_t rssi = 0;
  if (radio == WIFI_OFF) {
    copyText(gMode, sizeof(gMode), "off");
    copyText(gIp, sizeof(gIp), "0.0.0.0");
  } else if (radio == WIFI_AP) {
    copyText(gMode, sizeof(gMode), "AP");
    copyText(gIp, sizeof(gIp), WiFi.softAPIP().toString().c_str());
  } else if (radio == WIFI_STA) {
    copyText(gMode, sizeof(gMode), "STA");
    if (WiFi.status() == WL_CONNECTED) {
      copyText(gIp, sizeof(gIp), WiFi.localIP().toString().c_str());
      rssi = static_cast<int8_t>(WiFi.RSSI());
    } else {
      copyText(gIp, sizeof(gIp), "0.0.0.0");
    }
  } else {
    copyText(gMode, sizeof(gMode), "AP+STA");
    if (WiFi.status() == WL_CONNECTED) {
      copyText(gIp, sizeof(gIp), WiFi.localIP().toString().c_str());
      rssi = static_cast<int8_t>(WiFi.RSSI());
    } else {
      copyText(gIp, sizeof(gIp), WiFi.softAPIP().toString().c_str());
    }
  }
  Telemetry::setWifiInfo(gMode, gIp, rssi);
}

struct RadioPlan {
  bool ap;
  bool sta;
};

RadioPlan planFor(const GatewayConfig &c) {
  const char *m = c.wifiMode[0] ? c.wifiMode : "auto";
  if (strEqualsIgnoreCase(m, "off") == 0) return {false, false};
  if (strEqualsIgnoreCase(m, "ap") == 0) return {true, false};
  if (strEqualsIgnoreCase(m, "sta") == 0) return {false, true};
  if (strEqualsIgnoreCase(m, "apsta") == 0) return {true, true};
  return {true, c.staSsid[0] != '\0'};
}

const char *staLink() {
  const wifi_mode_t radio = WiFi.getMode();
  if ((radio & WIFI_STA) == 0) return "not started";
  if (!Settings::get().staSsid[0]) return "no SSID configured";
  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SSID_AVAIL:
      return "not found";
    case WL_CONNECT_FAILED:
      return "authentication failed";
    case WL_CONNECTION_LOST:
      return "connection lost";
    case WL_IDLE_STATUS:
      return "connecting";
    case WL_DISCONNECTED:
      return "disconnected";
    default:
      return "not connected";
  }
}

void printMac(const uint8_t mac[6]) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
}

const char *resetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      return "power-on";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "task watchdog";
    case ESP_RST_WDT:
      return "watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}

const char *authName(wifi_auth_mode_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN:
      return "open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/WPA3";
    default:
      return "secured";
  }
}

}  // namespace

namespace WifiPortal {

void startRadio() {
  const GatewayConfig &c = Settings::get();
  if (c.apSsid[0]) {
    copyText(gApSsid, sizeof(gApSsid), c.apSsid);
  } else {
    buildApSsid();
  }

  const RadioPlan plan = planFor(c);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setHostname(Settings::hostname());

  wifi_mode_t mode = WIFI_OFF;
  if (plan.ap && plan.sta) {
    mode = WIFI_AP_STA;
  } else if (plan.ap) {
    mode = WIFI_AP;
  } else if (plan.sta) {
    mode = WIFI_STA;
  }
  WiFi.mode(mode);
  WiFi.setAutoReconnect(plan.sta);

  if (!plan.sta) {
    WiFi.disconnect(false, false);
  } else if (c.staSsid[0]) {
    WiFi.begin(c.staSsid, c.staPass);
  }

  if (plan.ap) {
    const bool openAp = strlen(c.apPass) < 8;
    if (openAp) {
      WiFi.softAP(gApSsid);
    } else {
      WiFi.softAP(gApSsid, c.apPass);
    }
    Serial.print(F("[WiFi] AP "));
    Serial.print(gApSsid);
    Serial.print(F("  "));
    Serial.println(WiFi.softAPIP());
    if (openAp) {
      Serial.println(F("[WiFi] AP is open (no password)."));
    }
  } else {
    WiFi.softAPdisconnect(true);
  }

  if (plan.sta) {
    Serial.print(F("[WiFi] STA "));
    if (c.staSsid[0]) {
      Serial.println(c.staSsid);
    } else {
      Serial.println(F("(no SSID configured)"));
    }
  }
  if (!plan.ap && !plan.sta) {
    Serial.println(F("[WiFi] radio off"));
  }

  refreshInfo();
}

void begin() {
  Settings::setWifiHook(startRadio);
  startRadio();
}

void poll() { refreshInfo(); }

const char *mode() { return gMode; }
const char *ssid() { return gApSsid; }
const char *ip() { return gIp; }
int8_t rssi() {
  if (WiFi.status() == WL_CONNECTED) {
    return static_cast<int8_t>(WiFi.RSSI());
  }
  return 0;
}

void printBrief() {
  refreshInfo();
  const wifi_mode_t radio = WiFi.getMode();
  Serial.print(F("Wi-Fi: AP "));
  if ((radio & WIFI_AP) == 0) {
    Serial.print(F("off"));
  } else {
    Serial.print(F("up "));
    Serial.print(WiFi.softAPIP());
  }
  Serial.print(F(" | STA "));
  if ((radio & WIFI_STA) == 0) {
    Serial.println(F("off"));
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("connected "));
    Serial.println(WiFi.localIP());
    return;
  }
  Serial.println(staLink());
}

void printLine() {
  refreshInfo();
  const wifi_mode_t radio = WiFi.getMode();
  Serial.print(F("radio "));
  Serial.print(gMode);
  Serial.print(F(" | AP "));
  if ((radio & WIFI_AP) == 0) {
    Serial.print(F("off"));
  } else {
    Serial.print(WiFi.softAPIP());
  }
  Serial.print(F(" | STA "));
  Serial.print(staLink());
  if (WiFi.status() == WL_CONNECTED && (radio & WIFI_STA) != 0) {
    Serial.print(' ');
    Serial.print(WiFi.localIP());
    Serial.print(' ');
    Serial.print(static_cast<int>(WiFi.RSSI()));
    Serial.print(F(" dBm"));
  }
  Serial.println();
}

void printBoard() {
  refreshInfo();
  uint8_t staMac[6] = {};
  uint8_t apMac[6] = {};
  WiFi.macAddress(staMac);
  WiFi.softAPmacAddress(apMac);
  const uint32_t up = millis() / 1000UL;

  Serial.println(F("## BOARD"));
  Serial.print(F("Hostname: "));
  Serial.println(Settings::hostname());
  Serial.print(F("Chip: "));
  Serial.print(ESP.getChipModel());
  Serial.print(F(" rev "));
  Serial.println(ESP.getChipRevision());
  Serial.print(F("CPU: "));
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(F(" MHz"));
  Serial.print(F("Flash: "));
  Serial.print(ESP.getFlashChipSize() / 1024UL);
  Serial.println(F(" KB"));
  Serial.print(F("Heap free: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(F(" bytes"));
  Serial.print(F("SDK: "));
  Serial.println(ESP.getSdkVersion());
  Serial.print(F("STA MAC: "));
  printMac(staMac);
  Serial.println();
  Serial.print(F("AP MAC: "));
  printMac(apMac);
  Serial.println();
  Serial.print(F("Reset: "));
  Serial.println(resetReason());
  Serial.print(F("Uptime: "));
  Serial.print(up / 3600UL);
  Serial.print(':');
  const uint32_t mins = (up / 60UL) % 60UL;
  const uint32_t secs = up % 60UL;
  if (mins < 10) Serial.print('0');
  Serial.print(mins);
  Serial.print(':');
  if (secs < 10) Serial.print('0');
  Serial.println(secs);
  printLine();
}

void printStatus() {
  refreshInfo();
  const GatewayConfig &c = Settings::get();
  const wifi_mode_t radio = WiFi.getMode();
  uint8_t staMac[6] = {};
  uint8_t apMac[6] = {};
  WiFi.macAddress(staMac);
  WiFi.softAPmacAddress(apMac);

  Serial.println(F("## WIFI"));
  Serial.print(F("Saved mode: "));
  Serial.println(c.wifiMode[0] ? c.wifiMode : "auto");
  Serial.print(F("Radio: "));
  Serial.println(gMode);
  Serial.print(F("Hostname: "));
  Serial.println(Settings::hostname());

  Serial.println(F("Access point:"));
  if ((radio & WIFI_AP) == 0) {
    Serial.println(F("  status: off"));
  } else {
    Serial.println(F("  status: up"));
    Serial.print(F("  SSID: "));
    Serial.println(WiFi.softAPSSID());
    Serial.print(F("  IP: "));
    Serial.println(WiFi.softAPIP());
    Serial.print(F("  MAC: "));
    printMac(apMac);
    Serial.println();
    Serial.print(F("  Clients: "));
    Serial.println(WiFi.softAPgetStationNum());
    Serial.print(F("  Channel: "));
    Serial.println(WiFi.channel());
  }

  Serial.println(F("Station:"));
  Serial.print(F("  status: "));
  Serial.println(staLink());
  Serial.print(F("  configured SSID: "));
  Serial.println(c.staSsid[0] ? c.staSsid : "(none)");
  Serial.print(F("  MAC: "));
  printMac(staMac);
  Serial.println();
  if ((radio & WIFI_STA) != 0 && WiFi.status() == WL_CONNECTED) {
    Serial.print(F("  joined SSID: "));
    Serial.println(WiFi.SSID());
    Serial.print(F("  BSSID: "));
    printMac(WiFi.BSSID());
    Serial.println();
    Serial.print(F("  IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("  Gateway: "));
    Serial.println(WiFi.gatewayIP());
    Serial.print(F("  Mask: "));
    Serial.println(WiFi.subnetMask());
    Serial.print(F("  DNS: "));
    Serial.println(WiFi.dnsIP());
    Serial.print(F("  RSSI: "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
    Serial.print(F("  Channel: "));
    Serial.println(WiFi.channel());
  } else {
    Serial.println(F("  IP: 0.0.0.0"));
    Serial.println(F("  RSSI: n/a"));
  }
  Serial.println(F("Commands: wifi mode ap|sta|apsta|auto|off"));
  Serial.println(F("          wifi join <ssid> [password]"));
  Serial.println(F("          wifi disconnect | wifi scan"));
}

void scan() {
  if (WiFi.getMode() == WIFI_OFF) {
    Serial.println(F("Radio is off. Use: wifi mode apsta"));
    return;
  }
  Serial.println(F("Scanning..."));
  const int n = WiFi.scanNetworks();
  if (n < 0) {
    Serial.println(F("Scan failed."));
    return;
  }
  if (n == 0) {
    Serial.println(F("No networks found."));
    WiFi.scanDelete();
    return;
  }
  const int shown = n > 20 ? 20 : n;
  for (int i = 0; i < shown; i++) {
    Serial.printf("%2d  %4d dBm  ch %-2d  %-10s  %s\n", i + 1, WiFi.RSSI(i),
                  WiFi.channel(i), authName(WiFi.encryptionType(i)),
                  WiFi.SSID(i).c_str());
  }
  if (n > shown) {
    Serial.print(F("("));
    Serial.print(n - shown);
    Serial.println(F(" more not shown)"));
  }
  WiFi.scanDelete();
}

void disconnectSta() {
  if ((WiFi.getMode() & WIFI_STA) == 0) {
    Serial.println(F("Station radio is not started."));
    return;
  }
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  refreshInfo();
  Serial.println(F("Station disconnected. Saved SSID is unchanged."));
  Serial.println(F("It joins again on reboot, or: wifi join <ssid> [password]"));
  printLine();
}

}  // namespace WifiPortal

#endif  // ARDUINO_ARCH_ESP32
