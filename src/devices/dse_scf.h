#pragma once

#include <stddef.h>
#include <stdint.h>

#include "modbus/modbus_client.h"

// DSE System Control Function (SCF) keys — write key + complement to
// holding registers 4104/4105 in one FC16 transaction.

struct DseScfCommand {
  const char *key;   // CLI name, e.g. "telemetry_start"
  uint16_t code;     // System control key (e.g. 35732)
  const char *name;  // human description
};

namespace DseScf {

constexpr uint16_t kRegKey = 4104;
constexpr uint16_t kRegKeyInv = 4105;
constexpr uint16_t kRegFlagsBase = 4096;
constexpr uint8_t kFlagRegCount = 8;
constexpr uint16_t kKeyBase = 35700;
constexpr uint16_t kKeyStop = 35700;  // NEVER write while DG is running

size_t count();
const DseScfCommand &at(size_t index);
const DseScfCommand *findByKey(const char *key);

void printList();

// Returns true if support-flag bit for this control key is set.
// flags[0..7] = registers 4096..4103.
bool isSupported(uint16_t scfKey, const uint16_t flags[kFlagRegCount]);

// FC16 write [key, 65535-key] to 4104/4105.
ModbusResponse writeKey(uint8_t slaveId, uint16_t scfKey,
                        const ModbusTxnInfo *info = nullptr);

}  // namespace DseScf
