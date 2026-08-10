#pragma once

#include <cstddef>
#include <stdint.h>

// Compile-time Modbus slave profiles on the shared RS485 bus.
// No register maps / scaling here — transport + identity only.

struct ModbusDeviceConfig {
  const char *id;      // short key for CLI: "dse7320", "et1641"
  const char *name;    // display name
  const char *model;
  const char *serial;  // may be nullptr
  uint8_t slaveId;
  uint32_t baudRate;
  uint8_t dataBits;  // typically 8
  uint8_t parity;    // 0=None, 1=Odd, 2=Even
  uint8_t stopBits;  // 1 or 2
};

class DeviceRegistry {
public:
  void begin();

  size_t count() const;
  const ModbusDeviceConfig &at(size_t index) const;
  const ModbusDeviceConfig &selected() const;
  size_t selectedIndex() const;

  // Select by index or by id/name substring (case-insensitive).
  bool selectIndex(size_t index);
  bool selectByKey(const char *key);

  void printList() const;
  void printSelected() const;

  static const char *parityName(uint8_t parity);

private:
  size_t _selected = 0;
};

extern DeviceRegistry devices;
