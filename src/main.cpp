#include <Arduino.h>

#include "cli/cli.h"
#include "config/config.h"
#include "modbus/modbus_client.h"
#include "rs485/rs485.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "net/http_api.h"
#include "net/wifi_portal.h"
#include "telemetry/snapshot.h"
#endif

void setup() {
  Serial.begin(USB_SERIAL_BAUD);
#if defined(ARDUINO_ARCH_ESP32)
  // Drop writes while nobody is listening. Once the host is open, the loop
  // raises this so console echo cannot be discarded.
  Serial.setTxTimeoutMs(0);
#endif

  rs485.begin();
  modbus.begin();
  cli.begin();

#if defined(ARDUINO_ARCH_ESP32)
  Telemetry::begin();
  WifiPortal::begin();
  HttpApi::begin();
#else
  cli.present();
#endif
}

void loop() {
#if defined(ARDUINO_ARCH_ESP32)
  // ESP32-C3 HWCDC: Serial's bool can flicker. Debounce so we don't spam the
  // banner or skip RX polling while the user is typing.
  static bool bannerShown = false;
  static uint32_t lastHostOpenMs = 0;
  const bool hostOpen = static_cast<bool>(Serial);

  if (hostOpen) {
    lastHostOpenMs = millis();
    if (!bannerShown) {
      Serial.setTxTimeoutMs(20);
      delay(150);
      Serial.println();
      Serial.println(F("[USB CDC connected]"));
      cli.present();
      bannerShown = true;
    }
  } else if (bannerShown && (millis() - lastHostOpenMs) > 3000) {
    bannerShown = false;
    Serial.setTxTimeoutMs(0);
    cli.onHostDown();
  }

  WifiPortal::poll();
  HttpApi::poll();
  Telemetry::poll();
#endif

  // Always poll RX — do not gate on Serial bool (flicker drops keystrokes).
  cli.poll();
}
