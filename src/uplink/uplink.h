#pragma once

#if defined(ARDUINO_ARCH_ESP32)

namespace Uplink {

void begin();
void poll();
void printStatus();
// Send the current batch on the next poll, even if it is not full.
void kick();

}  // namespace Uplink

#endif
