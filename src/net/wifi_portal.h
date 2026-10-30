#pragma once

#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP32)

namespace WifiPortal {

void begin();
void poll();  // refresh Telemetry Wi-Fi fields

const char *mode();  // "AP" | "STA" | "AP+STA" | "off"
const char *ssid();
const char *ip();
int8_t rssi();

void printLine();
void printBrief();
void printStatus();
void printBoard();
void scan();
void disconnectSta();

}  // namespace WifiPortal

#endif  // ARDUINO_ARCH_ESP32
