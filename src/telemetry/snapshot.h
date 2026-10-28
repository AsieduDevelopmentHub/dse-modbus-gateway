#pragma once

#include <stddef.h>
#include <stdint.h>

// Live DSE overview snapshot for HTTP / React (ESP32). Refreshed from loop().

struct TelemetryAlarm {
  uint16_t address = 0;
  uint8_t slot = 0;
  uint8_t nibble = 0;
  char name[40] = {};
  char state[20] = {};
};

struct TelemetrySnapshot {
  bool ok = false;
  bool busy = false;
  uint32_t refreshedAtMs = 0;
  uint32_t ageMs = 0;
  uint8_t slaveId = 0;
  int8_t rssi = 0;
  char wifiMode[8] = {};
  char wifiIp[16] = {};
  uint32_t uptimeMs = 0;
  char lastError[48] = {};
  char link[12] = "unknown";  // ok | degraded | fail | unknown
  char headline[16] = "Offline";

  bool hasOpMode = false;
  uint16_t opModeCode = 0;
  char opModeName[28] = {};

  bool hasEngStatus = false;
  uint16_t engStatusCode = 0;
  char engStatusName[28] = {};

  bool hasEngRpm = false;
  float engRpm = 0;
  bool hasBatteryV = false;
  float batteryV = 0;
  bool hasGenFreq = false;
  float genFreq = 0;
  bool hasGenVan = false;
  float genVan = 0;
  bool hasGenVbn = false;
  float genVbn = 0;
  bool hasGenVcn = false;
  float genVcn = 0;
  bool hasGenIa = false;
  float genIa = 0;
  bool hasGenIb = false;
  float genIb = 0;
  bool hasGenIc = false;
  float genIc = 0;
  bool hasGenPTot = false;
  float genPTotW = 0;
  bool hasGenPctKw = false;
  float genPctKw = 0;

  TelemetryAlarm alarms[16];
  uint8_t alarmCount = 0;
};

namespace Telemetry {

void begin();
void poll();  // call from loop(); refreshes every few seconds when idle
void setAutoRefresh(bool enabled);
bool autoRefresh();
const TelemetrySnapshot &get();
void setWifiInfo(const char *mode, const char *ip, int8_t rssi);

// Write /api/status JSON into buf. Returns bytes written (excl. NUL), or 0.
size_t writeStatusJson(char *buf, size_t bufSize);
size_t writeHealthJson(char *buf, size_t bufSize);

}  // namespace Telemetry
