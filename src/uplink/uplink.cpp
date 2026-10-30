#include "uplink/uplink.h"

#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <esp_system.h>
#include <mbedtls/gcm.h>
#include <stdio.h>
#include <string.h>

#include "config/settings.h"
#include "telemetry/snapshot.h"

namespace {

constexpr size_t kMaxSamples = 8;
constexpr size_t kPlainMax = 768;
constexpr uint32_t kHeapFloor = 24576;
constexpr uint32_t kConnectMs = 3000;
constexpr uint32_t kBackoffCapS = 900;

struct Sample {
  uint32_t tMs = 0;
  uint16_t ageS = 0;
  uint8_t link = 0;
  uint8_t flags = 0;
  uint8_t mode = 0;
  uint8_t alarms = 0;
  int16_t rpm = 0;
  int16_t batt10 = 0;
  int16_t hz10 = 0;
  int16_t kw100 = 0;
  int16_t van10 = 0;
};

constexpr uint8_t kHasRpm = 1;
constexpr uint8_t kHasBatt = 2;
constexpr uint8_t kHasHz = 4;
constexpr uint8_t kHasKw = 8;
constexpr uint8_t kHasVan = 16;
constexpr uint8_t kHasMode = 32;

Sample gSamples[kMaxSamples];
uint8_t gCount = 0;
uint32_t gBoot = 0;
uint32_t gSeq = 0;
uint32_t gSent = 0;
uint32_t gFailed = 0;
uint32_t gDropped = 0;
uint32_t gLastSampleMs = 0;
uint32_t gNextTryMs = 0;
uint32_t gBackoffS = 5;
bool gForce = false;
char gLast[40] = "idle";

uint8_t linkCode(const char *link) {
  if (!link) return 0;
  if (strcmp(link, "ok") == 0) return 1;
  if (strcmp(link, "degraded") == 0) return 2;
  if (strcmp(link, "fail") == 0) return 3;
  return 0;
}

const char *linkName(uint8_t code) {
  if (code == 1) return "ok";
  if (code == 2) return "degraded";
  if (code == 3) return "fail";
  return "unknown";
}

void capture(const TelemetrySnapshot &snap) {
  if (gCount == kMaxSamples) {
    memmove(&gSamples[0], &gSamples[1], (kMaxSamples - 1) * sizeof(Sample));
    gCount = kMaxSamples - 1;
    gDropped++;
  }
  Sample &s = gSamples[gCount++];
  s = Sample{};
  s.tMs = snap.uptimeMs;
  s.ageS = snap.ageMs > 65535000ul ? 65535 : static_cast<uint16_t>(snap.ageMs / 1000ul);
  s.link = linkCode(snap.link);
  s.alarms = snap.alarmCount;
  if (snap.hasOpMode) {
    s.flags |= kHasMode;
    s.mode = static_cast<uint8_t>(snap.opModeCode > 255 ? 255 : snap.opModeCode);
  }
  if (snap.hasEngRpm) {
    s.flags |= kHasRpm;
    s.rpm = snap.engRpm > 32000.f ? 32000 : static_cast<int16_t>(snap.engRpm);
  }
  if (snap.hasBatteryV) {
    s.flags |= kHasBatt;
    s.batt10 = static_cast<int16_t>(snap.batteryV * 10.f);
  }
  if (snap.hasGenFreq) {
    s.flags |= kHasHz;
    s.hz10 = static_cast<int16_t>(snap.genFreq * 10.f);
  }
  if (snap.hasGenPTot) {
    s.flags |= kHasKw;
    s.kw100 = static_cast<int16_t>((snap.genPTotW / 1000.f) * 100.f);
  }
  if (snap.hasGenVan) {
    s.flags |= kHasVan;
    s.van10 = static_cast<int16_t>(snap.genVan * 10.f);
  }
}

int appendOpt(char *p, char *end, const char *key, bool present, long value) {
  if (present) {
    return snprintf(p, static_cast<size_t>(end - p), "\"%s\":%ld,", key, value);
  }
  return snprintf(p, static_cast<size_t>(end - p), "\"%s\":null,", key);
}

size_t buildPlain(char *buf, size_t cap, uint32_t seq) {
  char *p = buf;
  char *end = buf + cap;
  p += snprintf(p, static_cast<size_t>(end - p),
                "{\"boot\":%lu,\"seq\":%lu,\"n\":%u,\"s\":[",
                static_cast<unsigned long>(gBoot),
                static_cast<unsigned long>(seq), gCount);
  for (uint8_t i = 0; i < gCount && p < end; i++) {
    const Sample &s = gSamples[i];
    p += snprintf(p, static_cast<size_t>(end - p),
                  "%s{\"t\":%lu,\"age\":%u,\"link\":\"%s\",\"alm\":%u,",
                  i ? "," : "", static_cast<unsigned long>(s.tMs), s.ageS,
                  linkName(s.link), s.alarms);
    p += appendOpt(p, end, "mode", s.flags & kHasMode, s.mode);
    p += appendOpt(p, end, "rpm", s.flags & kHasRpm, s.rpm);
    p += appendOpt(p, end, "batt10", s.flags & kHasBatt, s.batt10);
    p += appendOpt(p, end, "hz10", s.flags & kHasHz, s.hz10);
    p += appendOpt(p, end, "kw100", s.flags & kHasKw, s.kw100);
    p += appendOpt(p, end, "van10", s.flags & kHasVan, s.van10);
    if (p > buf && p[-1] == ',') {
      p--;
    }
    p += snprintf(p, static_cast<size_t>(end - p), "}");
  }
  p += snprintf(p, static_cast<size_t>(end - p), "]}");
  if (p >= end) {
    return 0;
  }
  return static_cast<size_t>(p - buf);
}

bool seal(const uint8_t *plain, size_t plainLen, uint8_t count, uint32_t seq,
          uint8_t *packet, size_t *packetLen) {
  const GatewayConfig &cfg = Settings::get();
  if (plainLen > kPlainMax || plainLen > 65535) {
    return false;
  }
  uint8_t mac[6] = {};
  const uint64_t efuse = ESP.getEfuseMac();
  memcpy(mac, &efuse, 6);
  packet[0] = 'N';
  packet[1] = 'C';
  packet[2] = 'U';
  packet[3] = '1';
  packet[4] = 1;
  packet[5] = count;
  memcpy(packet + 6, mac, 6);
  memcpy(packet + 12, &seq, 4);
  esp_fill_random(packet + 16, 12);
  packet[28] = static_cast<uint8_t>(plainLen & 0xff);
  packet[29] = static_cast<uint8_t>((plainLen >> 8) & 0xff);

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  const int keyRc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, cfg.uplinkKey,
                                       128);
  uint8_t tag[16];
  const int cryptRc =
      keyRc == 0 ? mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plainLen,
                                            packet + 16, 12, packet, 30, plain,
                                            packet + 30, 16, tag)
                 : -1;
  mbedtls_gcm_free(&gcm);
  if (cryptRc != 0) {
    return false;
  }
  memcpy(packet + 30 + plainLen, tag, 16);
  *packetLen = 30 + plainLen + 16;
  return true;
}

void noteFail(const char *why) {
  gFailed++;
  snprintf(gLast, sizeof(gLast), "%s", why);
  gNextTryMs = millis() + gBackoffS * 1000ul;
  const uint32_t step =
      gBackoffS > (kBackoffCapS / 3) ? kBackoffCapS : gBackoffS * 3;
  gBackoffS = step > kBackoffCapS ? kBackoffCapS : step;
}

struct HttpReply {
  int code;
  char why[24];
};

void setWhy(HttpReply *reply, const char *why) {
  snprintf(reply->why, sizeof(reply->why), "%s", why);
}

HttpReply httpPost(const uint8_t *packet, size_t packetLen) {
  HttpReply reply{};
  reply.code = 0;
  const GatewayConfig &cfg = Settings::get();
  WiFiClient client;
  client.setTimeout(3);
  if (!client.connect(cfg.uplinkHost, cfg.uplinkPort, kConnectMs)) {
    setWhy(&reply, "connect failed");
    client.stop();
    return reply;
  }
  char head[180];
  const int headN = snprintf(
      head, sizeof(head),
      "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/octet-stream\r\n"
      "Content-Length: %u\r\nConnection: close\r\n\r\n",
      cfg.uplinkPath, cfg.uplinkHost, static_cast<unsigned>(packetLen));
  if (headN < 0 || headN >= static_cast<int>(sizeof(head)) ||
      client.write(reinterpret_cast<const uint8_t *>(head), headN) !=
          static_cast<size_t>(headN) ||
      client.write(packet, packetLen) != packetLen) {
    setWhy(&reply, "send failed");
    client.stop();
    return reply;
  }
  char line[64];
  const int got = client.readBytesUntil('\n', line, sizeof(line) - 1);
  client.stop();
  if (got <= 0) {
    setWhy(&reply, "no response");
    return reply;
  }
  line[got] = '\0';
  int code = 0;
  if (sscanf(line, "HTTP/%*s %d", &code) != 1) {
    setWhy(&reply, "bad status");
    return reply;
  }
  reply.code = code;
  snprintf(reply.why, sizeof(reply.why), "http %d", code);
  return reply;
}

bool postBatch(const uint8_t *packet, size_t packetLen) {
  if (ESP.getFreeHeap() < kHeapFloor) {
    noteFail("low heap");
    return false;
  }
  const HttpReply reply = httpPost(packet, packetLen);
  if (reply.code < 200 || reply.code > 299) {
    noteFail(reply.why[0] ? reply.why : "send failed");
    return false;
  }
  const GatewayConfig &cfg = Settings::get();
  gSent++;
  gSeq++;
  gCount = 0;
  gBackoffS = 5;
  gForce = false;
  snprintf(gLast, sizeof(gLast), "http %d", reply.code);
  gNextTryMs = millis() + static_cast<uint32_t>(cfg.uplinkPeriodS) * 1000ul;
  return true;
}

bool stationUp() {
  return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

}  // namespace

namespace Uplink {

void begin() {
  gBoot = esp_random();
  if (gBoot == 0) {
    gBoot = 1;
  }
  snprintf(gLast, sizeof(gLast), "idle");
}

void kick() { gForce = true; }

void heartbeat() {
  const GatewayConfig &cfg = Settings::get();
  if (!cfg.uplinkOn) {
    Serial.println(F("Uplink is off."));
  }
  if (!cfg.uplinkHost[0]) {
    Serial.println(F("Heartbeat failed: no host."));
    return;
  }
  if (!cfg.uplinkKeySet) {
    Serial.println(F("Heartbeat failed: key missing."));
    return;
  }
  if (!stationUp()) {
    Serial.println(F("Heartbeat failed: STA not connected."));
    return;
  }
  if (ESP.getFreeHeap() < kHeapFloor) {
    Serial.println(F("Heartbeat failed: low heap."));
    return;
  }
  char plain[96];
  const int plainN = snprintf(plain, sizeof(plain),
                               "{\"kind\":\"heartbeat\",\"boot\":%lu,\"seq\":0,\"n\":0}",
                               static_cast<unsigned long>(gBoot));
  if (plainN <= 0 || plainN >= static_cast<int>(sizeof(plain))) {
    Serial.println(F("Heartbeat failed: packet."));
    return;
  }
  static uint8_t packet[30 + 96 + 16];
  size_t packetLen = 0;
  if (!seal(reinterpret_cast<uint8_t *>(plain), static_cast<size_t>(plainN), 0, 0,
            packet, &packetLen)) {
    Serial.println(F("Heartbeat failed: encrypt."));
    return;
  }
  Serial.print(F("Heartbeat "));
  Serial.print(cfg.uplinkHost);
  Serial.print(':');
  Serial.print(cfg.uplinkPort);
  Serial.print(' ');
  Serial.println(cfg.uplinkPath);
  const HttpReply reply = httpPost(packet, packetLen);
  if (reply.code >= 200 && reply.code <= 299) {
    Serial.print(F("Heartbeat ok: "));
    Serial.println(reply.why);
    return;
  }
  Serial.print(F("Heartbeat failed: "));
  Serial.println(reply.why[0] ? reply.why : "send failed");
}

void printStatus() {
  const GatewayConfig &cfg = Settings::get();
  const uint32_t now = millis();
  Serial.print(F("uplink: "));
  Serial.println(cfg.uplinkOn ? F("on") : F("off"));
  Serial.print(F("host: "));
  if (cfg.uplinkHost[0]) {
    Serial.print(cfg.uplinkHost);
    Serial.print(':');
    Serial.print(cfg.uplinkPort);
    Serial.println(cfg.uplinkPath);
  } else {
    Serial.println(F("(none)"));
  }
  Serial.print(F("key: "));
  Serial.println(cfg.uplinkKeySet ? F("set") : F("missing"));
  Serial.print(F("batch: "));
  Serial.print(gCount);
  Serial.print('/');
  Serial.print(cfg.uplinkBatch);
  Serial.print(F("  period: "));
  Serial.print(cfg.uplinkPeriodS);
  Serial.println(F("s"));
  Serial.print(F("seq: "));
  Serial.print(gSeq);
  Serial.print(F("  sent: "));
  Serial.print(gSent);
  Serial.print(F("  failed: "));
  Serial.print(gFailed);
  Serial.print(F("  dropped: "));
  Serial.println(gDropped);
  Serial.print(F("station: "));
  Serial.println(stationUp() ? F("up") : F("down"));
  Serial.print(F("heap: "));
  Serial.println(ESP.getFreeHeap());
  Serial.print(F("last: "));
  Serial.println(gLast);
  if (gNextTryMs > now) {
    Serial.print(F("next try: "));
    Serial.print((gNextTryMs - now) / 1000ul);
    Serial.println(F("s"));
  }
}

void poll() {
  const GatewayConfig &cfg = Settings::get();
  const uint32_t now = millis();
  const TelemetrySnapshot &snap = Telemetry::get();
  if (snap.busy) {
    return;
  }
  if (cfg.uplinkOn && snap.refreshedAtMs != 0) {
    const uint32_t periodMs = static_cast<uint32_t>(cfg.uplinkPeriodS) * 1000ul;
    if (gLastSampleMs == 0 || (now - gLastSampleMs) >= periodMs) {
      capture(snap);
      gLastSampleMs = now;
    }
  }
  if (!cfg.uplinkOn || !cfg.uplinkKeySet || !cfg.uplinkHost[0] || gCount == 0) {
    return;
  }
  const uint8_t want = cfg.uplinkBatch == 0 ? 1 : cfg.uplinkBatch;
  if (!gForce && gCount < want) {
    return;
  }
  if (!gForce && gNextTryMs != 0 && now < gNextTryMs) {
    return;
  }
  if (!stationUp()) {
    snprintf(gLast, sizeof(gLast), "no station");
    gNextTryMs = now + 15000ul;
    return;
  }
  char plain[kPlainMax];
  const size_t plainLen = buildPlain(plain, sizeof(plain), gSeq + 1);
  if (plainLen == 0) {
    noteFail("batch too large");
    return;
  }
  static uint8_t packet[30 + kPlainMax + 16];
  size_t packetLen = 0;
  if (!seal(reinterpret_cast<uint8_t *>(plain), plainLen, gCount, gSeq + 1, packet,
            &packetLen)) {
    noteFail("encrypt failed");
    return;
  }
  postBatch(packet, packetLen);
}

}  // namespace Uplink

#endif
