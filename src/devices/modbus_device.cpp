#include "devices/modbus_device.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config/config.h"

// Static profiles — unique slave IDs on one RS485 bus.
// Do NOT put guessed register maps here.
static const ModbusDeviceConfig kDevices[] = {
    {
        "dse7320",
        "DSE 7320 / 7320 MKII",
        "DSE 73xx family",
        nullptr,
        MODBUS_SLAVE_ID,  // default 1 @ 9600 (CLI: set slave / set baud)
        MODBUS_BAUD_RATE,
        MODBUS_DATA_BITS,
        MODBUS_PARITY,
        MODBUS_STOP_BITS,
    },
    {
        "et1641",
        "Invendis ET1641",
        "ET1641",
        "725093",
        50,
        9600,
        8,
        0,  // none
        1,
    },
};

static constexpr size_t kDeviceCount =
    sizeof(kDevices) / sizeof(kDevices[0]);

DeviceRegistry devices;

void DeviceRegistry::begin() {
  _selected = 0;  // DSE remains the default profile
}

size_t DeviceRegistry::count() const { return kDeviceCount; }

const ModbusDeviceConfig &DeviceRegistry::at(size_t index) const {
  return kDevices[index < kDeviceCount ? index : 0];
}

const ModbusDeviceConfig &DeviceRegistry::selected() const {
  return kDevices[_selected];
}

size_t DeviceRegistry::selectedIndex() const { return _selected; }

bool DeviceRegistry::selectIndex(size_t index) {
  if (index >= kDeviceCount) {
    return false;
  }
  _selected = index;
  return true;
}

bool DeviceRegistry::selectByKey(const char *key) {
  if (!key || !*key) {
    return false;
  }

  // Numeric index?
  char *end = nullptr;
  const long asIndex = strtol(key, &end, 10);
  if (end && *end == '\0' && asIndex >= 0 &&
      static_cast<size_t>(asIndex) < kDeviceCount) {
    _selected = static_cast<size_t>(asIndex);
    return true;
  }

  for (size_t i = 0; i < kDeviceCount; i++) {
    if (strcasecmp(key, kDevices[i].id) == 0 ||
        strcasecmp(key, kDevices[i].model) == 0) {
      _selected = i;
      return true;
    }
    // Allow partial match on display name, e.g. "et1641", "dse", "invendis"
    if (strcasestr(kDevices[i].name, key) != nullptr ||
        strcasestr(kDevices[i].id, key) != nullptr) {
      _selected = i;
      return true;
    }
  }
  return false;
}

const char *DeviceRegistry::parityName(uint8_t parity) {
  switch (parity) {
    case 1:
      return "Odd";
    case 2:
      return "Even";
    default:
      return "None";
  }
}

void DeviceRegistry::printList() const {
  Serial.println(F("Available devices:"));
  Serial.println();
  for (size_t i = 0; i < kDeviceCount; i++) {
    const ModbusDeviceConfig &d = kDevices[i];
    Serial.print(F("["));
    Serial.print(i);
    Serial.print(F("] "));
    Serial.print(d.name);
    if (i == _selected) {
      Serial.print(F("  <-- selected"));
    }
    Serial.println();
    Serial.print(F("    ID key:   "));
    Serial.println(d.id);
    Serial.print(F("    Model:    "));
    Serial.println(d.model);
    if (d.serial) {
      Serial.print(F("    Serial:   "));
      Serial.println(d.serial);
    }
    Serial.print(F("    Slave ID: "));
    Serial.println(d.slaveId);
    Serial.print(F("    Baud:     "));
    Serial.println(d.baudRate);
    Serial.print(F("    Format:   "));
    Serial.print(d.dataBits);
    Serial.print(d.parity == 1 ? 'O' : (d.parity == 2 ? 'E' : 'N'));
    Serial.println(d.stopBits);
    Serial.println();
  }
}

void DeviceRegistry::printSelected() const {
  const ModbusDeviceConfig &d = selected();
  Serial.println(F("Selected device:"));
  Serial.print(F("  Device:    "));
  Serial.println(d.name);
  Serial.print(F("  Model:     "));
  Serial.println(d.model);
  if (d.serial) {
    Serial.print(F("  Serial:    "));
    Serial.println(d.serial);
  }
  Serial.print(F("  Key:       "));
  Serial.println(d.id);
  Serial.print(F("  Slave ID:  "));
  Serial.println(d.slaveId);
  Serial.print(F("  Baud:      "));
  Serial.println(d.baudRate);
  Serial.print(F("  Parity:    "));
  Serial.println(parityName(d.parity));
  Serial.print(F("  Stop bits: "));
  Serial.println(d.stopBits);
  Serial.print(F("  Format:    "));
  Serial.print(d.dataBits);
  Serial.print(d.parity == 1 ? 'O' : (d.parity == 2 ? 'E' : 'N'));
  Serial.println(d.stopBits);
}
