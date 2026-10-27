#if defined(ARDUINO_ARCH_ESP32)

#include "net/http_api.h"

#include <WebServer.h>
#include <WiFi.h>

#include "config/config.h"
#include "net/overview_html.h"
#include "telemetry/snapshot.h"

namespace {

WebServer server(HTTP_SERVER_PORT);
char jsonBuf[4096];

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  sendCors();
  server.send(204);
}

void handleRoot() {
  server.send_P(200, "text/html", OVERVIEW_HTML);
}

void handleStatus() {
  sendCors();
  // Avoid Modbus refresh mid-request; serve last snapshot only.
  const size_t n = Telemetry::writeStatusJson(jsonBuf, sizeof(jsonBuf));
  if (n == 0) {
    server.send(500, "application/json", "{\"error\":\"json\"}");
    return;
  }
  server.send(200, "application/json", jsonBuf);
}

void handleHealth() {
  sendCors();
  const size_t n = Telemetry::writeHealthJson(jsonBuf, sizeof(jsonBuf));
  if (n == 0) {
    server.send(500, "application/json", "{\"error\":\"json\"}");
    return;
  }
  server.send(200, "application/json", jsonBuf);
}

void handleNotFound() {
  sendCors();
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

}  // namespace

namespace HttpApi {

void begin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/health", HTTP_GET, handleHealth);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/health", HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.print(F("[HTTP] Listening on :"));
  Serial.println(HTTP_SERVER_PORT);
}

void poll() { server.handleClient(); }

}  // namespace HttpApi

#endif  // ARDUINO_ARCH_ESP32
