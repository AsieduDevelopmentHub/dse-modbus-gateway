#pragma once

#if defined(ARDUINO_ARCH_ESP32)

namespace HttpApi {

void begin();
void poll();  // WebServer.handleClient()

}  // namespace HttpApi

#endif  // ARDUINO_ARCH_ESP32
