#include <Arduino.h>

#include "cli/cli.h"
#include "config/config.h"
#include "modbus/modbus_client.h"
#include "rs485/rs485.h"

void setup() {
  // Native USB CDC (no CP210x/CH340). Baud is ignored by the host link.
  Serial.begin(USB_SERIAL_BAUD);
  Serial.setTxTimeoutMs(0);  // don't block forever if host isn't open yet

  rs485.begin();
  modbus.begin();
  cli.begin();  // may print into the void if monitor not open yet — OK
}

void loop() {
  // ESP32-C3 HWCDC: Serial's bool can flicker. Debounce so we don't spam the
  // banner or skip RX polling while the user is typing.
  static bool bannerShown = false;
  static uint32_t lastHostOpenMs = 0;
  const bool hostOpen = static_cast<bool>(Serial);

  if (hostOpen) {
    lastHostOpenMs = millis();
    if (!bannerShown) {
      delay(150);
      Serial.println();
      Serial.println(F("[USB CDC connected]"));
      cli.printBanner();
      bannerShown = true;
    }
  } else if (bannerShown && (millis() - lastHostOpenMs) > 3000) {
    bannerShown = false;
  }

  // Always poll RX — do not gate on Serial bool (flicker drops keystrokes).
  cli.poll();
}
