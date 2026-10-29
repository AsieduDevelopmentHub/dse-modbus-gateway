#include "config/settings.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "util/str_util.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <LittleFS.h>

#include "devices/modbus_device.h"
#include "fs/vfs.h"
#include "rs485/rs485.h"
#include "telemetry/snapshot.h"

namespace {

constexpr const char *kPath = "/etc/gateway.conf";

GatewayConfig gCfg;
bool gLoaded = false;
void (*gWifiHook)() = nullptr;

const char *kKeys[] = {
    "hostname",
    "console.idle_min",
    "modbus.device",
    "modbus.slave",
    "modbus.baud",
    "modbus.parity",
    "modbus.fc",
    "modbus.settle_us",
    "modbus.predelay_us",
    "modbus.telemetry",
    "wifi.ap_ssid",
    "wifi.ap_pass",
    "wifi.sta_ssid",
    "wifi.sta_pass",
    nullptr,
};

void setErr(char *err, size_t n, const char *msg) {
  if (err && n) {
    snprintf(err, n, "%s", msg);
  }
}

char *trim(char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\r') {
    s++;
  }
  char *e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' ||
                   e[-1] == '\n')) {
    *--e = '\0';
  }
  return s;
}

const char *parityStr(uint8_t p) {
  if (p == 1) return "odd";
  if (p == 2) return "even";
  return "none";
}

const char *canonical(const char *key) {
  if (!key) return nullptr;
  for (size_t i = 0; kKeys[i]; i++) {
    if (strEqualsIgnoreCase(kKeys[i], key) == 0) {
      return kKeys[i];
    }
  }
  return nullptr;
}

bool parseU32(const char *s, uint32_t *out) {
  if (!s || !*s) return false;
  char *end = nullptr;
  const unsigned long v = strtoul(s, &end, 10);
  if (end == s || (end && *end)) return false;
  *out = static_cast<uint32_t>(v);
  return true;
}

bool copyText(char *dst, size_t dstN, const char *value, size_t maxLen,
              bool allowEmpty) {
  const size_t n = strlen(value);
  if (n > maxLen || n + 1 > dstN) return false;
  if (!allowEmpty && n == 0) return false;
  memcpy(dst, value, n + 1);
  return true;
}

void setDefaults(GatewayConfig *c) {
  memset(c, 0, sizeof(*c));
  strncpy(c->hostname, "dse-gw", sizeof(c->hostname) - 1);
  strncpy(c->device, "dse7320", sizeof(c->device) - 1);
  c->slave = MODBUS_SLAVE_ID;
  c->baud = MODBUS_BAUD_RATE;
  c->parity = MODBUS_PARITY;
  c->fc = 0x03;
  c->settleUs = RS485_TX_SETTLE_US;
  c->predelayUs = RS485_TX_PRE_DELAY_US;
  c->telemetry = false;
  strncpy(c->apPass, WIFI_AP_PASSWORD, sizeof(c->apPass) - 1);
  c->idleMin = 30;
}

bool assign(GatewayConfig *c, const char *key, const char *value, char *err,
            size_t errN) {
  if (strchr(value, '\n') || strchr(value, '\r')) {
    setErr(err, errN, "value cannot contain a line break");
    return false;
  }
  if (strcmp(key, "hostname") == 0) {
    const size_t n = strlen(value);
    if (n < 1 || n > 23) {
      setErr(err, errN, "hostname must be 1..23 characters");
      return false;
    }
    for (size_t i = 0; i < n; i++) {
      const char ch = value[i];
      const bool ok = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                      (ch >= '0' && ch <= '9') || ch == '-';
      if (!ok) {
        setErr(err, errN, "hostname may use letters, digits, and hyphen");
        return false;
      }
    }
    strncpy(c->hostname, value, sizeof(c->hostname) - 1);
    c->hostname[sizeof(c->hostname) - 1] = '\0';
    return true;
  }
  if (strcmp(key, "console.idle_min") == 0) {
    uint32_t v = 0;
    if (!parseU32(value, &v) || v > 1440) {
      setErr(err, errN, "console.idle_min must be 0..1440");
      return false;
    }
    c->idleMin = static_cast<uint16_t>(v);
    return true;
  }
  if (strcmp(key, "modbus.device") == 0) {
    const size_t prev = devices.selectedIndex();
    if (!devices.selectByKey(value)) {
      devices.selectIndex(prev);
      setErr(err, errN, "unknown device");
      return false;
    }
    const ModbusDeviceConfig &d = devices.selected();
    strncpy(c->device, d.id, sizeof(c->device) - 1);
    c->device[sizeof(c->device) - 1] = '\0';
    c->slave = d.slaveId;
    c->baud = d.baudRate;
    c->parity = d.parity;
    devices.selectIndex(prev);
    return true;
  }
  if (strcmp(key, "modbus.slave") == 0) {
    uint32_t v = 0;
    if (!parseU32(value, &v) || v < 1 || v > 247) {
      setErr(err, errN, "slave must be 1..247");
      return false;
    }
    c->slave = static_cast<uint8_t>(v);
    return true;
  }
  if (strcmp(key, "modbus.baud") == 0) {
    uint32_t v = 0;
    if (!parseU32(value, &v) || (v != 4800 && v != 9600 && v != 19200)) {
      setErr(err, errN, "baud must be 4800, 9600, or 19200");
      return false;
    }
    c->baud = v;
    return true;
  }
  if (strcmp(key, "modbus.parity") == 0) {
    if (strEqualsIgnoreCase(value, "none") == 0 || strcmp(value, "0") == 0) {
      c->parity = 0;
    } else if (strEqualsIgnoreCase(value, "odd") == 0 || strcmp(value, "1") == 0) {
      c->parity = 1;
    } else if (strEqualsIgnoreCase(value, "even") == 0 || strcmp(value, "2") == 0) {
      c->parity = 2;
    } else {
      setErr(err, errN, "parity must be none, even, or odd");
      return false;
    }
    return true;
  }
  if (strcmp(key, "modbus.fc") == 0) {
    const long fc = strtol(value, nullptr, 16);
    if (fc != 0x03 && fc != 0x04) {
      setErr(err, errN, "fc must be 03 or 04");
      return false;
    }
    c->fc = static_cast<uint8_t>(fc);
    return true;
  }
  if (strcmp(key, "modbus.settle_us") == 0) {
    uint32_t v = 0;
    if (!parseU32(value, &v) || v > 20000) {
      setErr(err, errN, "settle must be 0..20000 us");
      return false;
    }
    c->settleUs = v;
    return true;
  }
  if (strcmp(key, "modbus.predelay_us") == 0) {
    uint32_t v = 0;
    if (!parseU32(value, &v) || v > 5000) {
      setErr(err, errN, "predelay must be 0..5000 us");
      return false;
    }
    c->predelayUs = v;
    return true;
  }
  if (strcmp(key, "modbus.telemetry") == 0) {
    if (strEqualsIgnoreCase(value, "on") == 0) {
      c->telemetry = true;
    } else if (strEqualsIgnoreCase(value, "off") == 0) {
      c->telemetry = false;
    } else {
      setErr(err, errN, "telemetry must be on or off");
      return false;
    }
    return true;
  }
  if (strcmp(key, "wifi.ap_ssid") == 0) {
    if (!copyText(c->apSsid, sizeof(c->apSsid), value, 32, true)) {
      setErr(err, errN, "wifi.ap_ssid must be 0..32 characters");
      return false;
    }
    return true;
  }
  if (strcmp(key, "wifi.ap_pass") == 0) {
    const size_t n = strlen(value);
    if (n != 0 && (n < 8 || n > 63)) {
      setErr(err, errN, "wifi.ap_pass must be empty or 8..63 characters");
      return false;
    }
    if (!copyText(c->apPass, sizeof(c->apPass), value, 63, true)) {
      setErr(err, errN, "wifi.ap_pass must be empty or 8..63 characters");
      return false;
    }
    return true;
  }
  if (strcmp(key, "wifi.sta_ssid") == 0) {
    if (!copyText(c->staSsid, sizeof(c->staSsid), value, 32, true)) {
      setErr(err, errN, "wifi.sta_ssid must be 0..32 characters");
      return false;
    }
    return true;
  }
  if (strcmp(key, "wifi.sta_pass") == 0) {
    if (!copyText(c->staPass, sizeof(c->staPass), value, 63, true)) {
      setErr(err, errN, "wifi.sta_pass must be 0..63 characters");
      return false;
    }
    return true;
  }
  setErr(err, errN, "unknown key");
  return false;
}

void applyBus() {
  if (gCfg.device[0]) {
    devices.selectByKey(gCfg.device);
  }
  rs485.applyConfig(gCfg.baud, MODBUS_DATA_BITS, gCfg.parity, MODBUS_STOP_BITS);
  rs485.setTxSettleUs(gCfg.settleUs);
  rs485.setTxPreDelayUs(gCfg.predelayUs);
}

void applyTelemetry() { Telemetry::setAutoRefresh(gCfg.telemetry); }

void applyKey(const char *key) {
  if (strcmp(key, "modbus.telemetry") == 0) {
    applyTelemetry();
    return;
  }
  if (strncmp(key, "wifi.", 5) == 0) {
    if (gWifiHook) {
      gWifiHook();
    }
    return;
  }
  if (strcmp(key, "modbus.baud") == 0 || strcmp(key, "modbus.parity") == 0 ||
      strcmp(key, "modbus.settle_us") == 0 ||
      strcmp(key, "modbus.predelay_us") == 0 ||
      strcmp(key, "modbus.device") == 0) {
    applyBus();
  }
}

bool writeFile() {
  if (!Vfs::mounted()) {
    return false;
  }
  LittleFS.mkdir("/etc");
  char body[800];
  const int n = snprintf(
      body, sizeof(body),
      "# gateway configuration\n"
      "hostname=%s\n"
      "console.idle_min=%u\n"
      "modbus.device=%s\n"
      "modbus.slave=%u\n"
      "modbus.baud=%lu\n"
      "modbus.parity=%s\n"
      "modbus.fc=%02x\n"
      "modbus.settle_us=%lu\n"
      "modbus.predelay_us=%lu\n"
      "modbus.telemetry=%s\n"
      "wifi.ap_ssid=%s\n"
      "wifi.ap_pass=%s\n"
      "wifi.sta_ssid=%s\n"
      "wifi.sta_pass=%s\n",
      gCfg.hostname, static_cast<unsigned>(gCfg.idleMin), gCfg.device,
      static_cast<unsigned>(gCfg.slave), static_cast<unsigned long>(gCfg.baud),
      parityStr(gCfg.parity), static_cast<unsigned>(gCfg.fc),
      static_cast<unsigned long>(gCfg.settleUs),
      static_cast<unsigned long>(gCfg.predelayUs),
      gCfg.telemetry ? "on" : "off", gCfg.apSsid, gCfg.apPass, gCfg.staSsid,
      gCfg.staPass);
  if (n < 0 || n >= static_cast<int>(sizeof(body))) {
    return false;
  }
  File f = LittleFS.open(kPath, "w");
  if (!f) {
    return false;
  }
  const size_t w = f.write(reinterpret_cast<const uint8_t *>(body),
                           static_cast<size_t>(n));
  f.close();
  return w == static_cast<size_t>(n);
}

bool readInto(GatewayConfig *dst) {
  File f = LittleFS.open(kPath, "r");
  if (!f) {
    return false;
  }
  GatewayConfig next;
  setDefaults(&next);
  while (f.available()) {
    char line[160];
    size_t i = 0;
    bool overflow = false;
    while (f.available()) {
      const int c = f.read();
      if (c < 0 || c == '\n') {
        break;
      }
      if (i + 1 >= sizeof(line)) {
        overflow = true;
        continue;
      }
      if (c != '\r') {
        line[i++] = static_cast<char>(c);
      }
    }
    line[i] = '\0';
    if (overflow) {
      Serial.println(F("config: skipped a line that was too long"));
      continue;
    }
    char *s = trim(line);
    if (!s[0] || s[0] == '#') {
      continue;
    }
    char *eq = strchr(s, '=');
    if (!eq) {
      Serial.print(F("config: skip "));
      Serial.println(s);
      continue;
    }
    *eq = '\0';
    char *k = trim(s);
    char *v = trim(eq + 1);
    const char *ck = canonical(k);
    if (!ck) {
      Serial.print(F("config: unknown key "));
      Serial.println(k);
      continue;
    }
    char err[72];
    err[0] = '\0';
    if (!assign(&next, ck, v, err, sizeof(err))) {
      Serial.print(F("config: "));
      Serial.println(err[0] ? err : "bad value");
    }
  }
  f.close();
  *dst = next;
  return true;
}

}  // namespace

namespace Settings {

bool begin() {
  setDefaults(&gCfg);
  gLoaded = false;
  if (!Vfs::begin()) {
    Serial.println(F("[fs] LittleFS mount failed; config will not persist"));
    applyBus();
    applyTelemetry();
    return false;
  }
  if (!LittleFS.exists(kPath)) {
    if (!writeFile()) {
      Serial.println(F("[fs] could not write /etc/gateway.conf"));
    }
  } else if (!readInto(&gCfg)) {
    Serial.println(F("[fs] could not read /etc/gateway.conf"));
    setDefaults(&gCfg);
  }
  gLoaded = true;
  applyBus();
  applyTelemetry();
  return true;
}

bool loaded() { return gLoaded; }

const GatewayConfig &get() { return gCfg; }

const char *hostname() {
  return gCfg.hostname[0] ? gCfg.hostname : "dse-gw";
}

const char *path() { return kPath; }

bool put(const char *key, const char *value, char *err, size_t errN) {
  if (err && errN) err[0] = '\0';
  const char *ck = canonical(key);
  if (!ck) {
    setErr(err, errN, "unknown key");
    return false;
  }
  GatewayConfig next = gCfg;
  if (!assign(&next, ck, value ? value : "", err, errN)) {
    return false;
  }
  gCfg = next;
  applyKey(ck);
  if (!gLoaded) {
    setErr(err, errN, "applied, filesystem not mounted");
    return false;
  }
  if (!writeFile()) {
    setErr(err, errN, "applied, could not save /etc/gateway.conf");
    return false;
  }
  return true;
}

bool reload(char *err, size_t errN) {
  if (!Vfs::mounted() || !LittleFS.exists(kPath)) {
    setErr(err, errN, "missing /etc/gateway.conf");
    return false;
  }
  if (!readInto(&gCfg)) {
    setErr(err, errN, "could not read /etc/gateway.conf");
    return false;
  }
  applyBus();
  applyTelemetry();
  if (gWifiHook) {
    gWifiHook();
  }
  return true;
}

bool save(char *err, size_t errN) {
  if (!writeFile()) {
    setErr(err, errN, "could not save /etc/gateway.conf");
    return false;
  }
  return true;
}

bool format(const char *key, char *out, size_t outN, bool maskSecrets) {
  if (!out || outN == 0) return false;
  const char *ck = canonical(key);
  if (!ck) return false;
  const bool secret = maskSecrets && strstr(ck, "pass") != nullptr;
  if (strcmp(ck, "hostname") == 0) {
    snprintf(out, outN, "%s", gCfg.hostname);
  } else if (strcmp(ck, "console.idle_min") == 0) {
    snprintf(out, outN, "%u", static_cast<unsigned>(gCfg.idleMin));
  } else if (strcmp(ck, "modbus.device") == 0) {
    snprintf(out, outN, "%s", gCfg.device);
  } else if (strcmp(ck, "modbus.slave") == 0) {
    snprintf(out, outN, "%u", static_cast<unsigned>(gCfg.slave));
  } else if (strcmp(ck, "modbus.baud") == 0) {
    snprintf(out, outN, "%lu", static_cast<unsigned long>(gCfg.baud));
  } else if (strcmp(ck, "modbus.parity") == 0) {
    snprintf(out, outN, "%s", parityStr(gCfg.parity));
  } else if (strcmp(ck, "modbus.fc") == 0) {
    snprintf(out, outN, "%02x", static_cast<unsigned>(gCfg.fc));
  } else if (strcmp(ck, "modbus.settle_us") == 0) {
    snprintf(out, outN, "%lu", static_cast<unsigned long>(gCfg.settleUs));
  } else if (strcmp(ck, "modbus.predelay_us") == 0) {
    snprintf(out, outN, "%lu", static_cast<unsigned long>(gCfg.predelayUs));
  } else if (strcmp(ck, "modbus.telemetry") == 0) {
    snprintf(out, outN, "%s", gCfg.telemetry ? "on" : "off");
  } else if (strcmp(ck, "wifi.ap_ssid") == 0) {
    snprintf(out, outN, "%s", gCfg.apSsid);
  } else if (strcmp(ck, "wifi.ap_pass") == 0) {
    snprintf(out, outN, "%s", (secret && gCfg.apPass[0]) ? "********" : gCfg.apPass);
  } else if (strcmp(ck, "wifi.sta_ssid") == 0) {
    snprintf(out, outN, "%s", gCfg.staSsid);
  } else if (strcmp(ck, "wifi.sta_pass") == 0) {
    snprintf(out, outN, "%s",
             (secret && gCfg.staPass[0]) ? "********" : gCfg.staPass);
  } else {
    return false;
  }
  return true;
}

void printKeys() {
  Serial.println(F("Keys (config set <key> <value>):"));
  for (size_t i = 0; kKeys[i]; i++) {
    Serial.print(F("  "));
    Serial.println(kKeys[i]);
  }
  Serial.println(F("config set rewrites the file. Hand edits last until then."));
  Serial.println(F("After editing the file: config reload"));
}

void printSummary() {
  Serial.println(F("## SAVED"));
  Serial.println(kPath);
  for (size_t i = 0; kKeys[i]; i++) {
    char val[68];
    if (!format(kKeys[i], val, sizeof(val), true)) {
      continue;
    }
    Serial.print(kKeys[i]);
    Serial.print('=');
    Serial.println(val);
  }
}

void setWifiHook(void (*fn)()) { gWifiHook = fn; }

}  // namespace Settings

#else

namespace {

GatewayConfig gCfg;
bool gInit = false;

void fill() {
  if (gInit) return;
  memset(&gCfg, 0, sizeof(gCfg));
  strncpy(gCfg.hostname, "dse-gw", sizeof(gCfg.hostname) - 1);
  gCfg.slave = MODBUS_SLAVE_ID;
  gCfg.baud = MODBUS_BAUD_RATE;
  gCfg.parity = MODBUS_PARITY;
  gCfg.fc = 0x03;
  gCfg.idleMin = 0;
  gInit = true;
}

}  // namespace

namespace Settings {

bool begin() {
  fill();
  return false;
}
bool loaded() { return false; }
const GatewayConfig &get() {
  fill();
  return gCfg;
}
const char *hostname() { return "dse-gw"; }
const char *path() { return ""; }
bool put(const char *, const char *, char *err, size_t errN) {
  if (err && errN) {
    snprintf(err, errN, "persistent config is on the ESP32 console");
  }
  return false;
}
bool reload(char *err, size_t errN) { return put(nullptr, nullptr, err, errN); }
bool save(char *err, size_t errN) { return put(nullptr, nullptr, err, errN); }
bool format(const char *, char *out, size_t, bool) {
  if (out) out[0] = '\0';
  return false;
}
void printKeys() {
  Serial.println(F("Persistent config is on the ESP32 console."));
}
void printSummary() {}
void setWifiHook(void (*)()) {}

}  // namespace Settings

#endif
