#pragma once

#include <stddef.h>
#include <stdint.h>

// Runtime configuration. On ESP32 the source of truth is /etc/gateway.conf.

struct GatewayConfig {
  char hostname[24];
  char device[16];
  uint8_t slave;
  uint32_t baud;
  uint8_t parity;
  uint8_t fc;
  uint32_t settleUs;
  uint32_t predelayUs;
  bool telemetry;
  char apSsid[33];
  char apPass[64];
  char staSsid[33];
  char staPass[64];
  uint16_t idleMin;
};

namespace Settings {

bool begin();
bool loaded();
const GatewayConfig &get();
const char *hostname();
const char *path();
bool put(const char *key, const char *value, char *err, size_t errN);
bool reload(char *err, size_t errN);
bool save(char *err, size_t errN);
bool format(const char *key, char *out, size_t outN, bool maskSecrets);
void printKeys();
void printSummary();
void setWifiHook(void (*fn)());

}  // namespace Settings
