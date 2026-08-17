#pragma once

#include <stddef.h>
#include <stdint.h>

#include "modbus/modbus_client.h"

// Deep Sea GenComm holding-register map (FC03).
// Sources: doc/dse_register_map/register_map.md (S6/7320)
//          doc/dse_register_map/register_clone_dse8610.md (GenComm 8610)
// 32-bit values: high-word first (page layout / Reverse order = No).

enum class DseDataWidth : uint8_t {
  Int16 = 1,
  Int32 = 2,
};

enum class DseDecodeKind : uint8_t {
  Scaled = 0,         // engineering = raw * factor
  EnumOpMode,         // register 772
  EnumEngStatus,      // register 1408
  AlarmPacked,        // 4×4-bit GenComm alarm slots
  Bitmask,            // SCF support flags etc.
  HoursFromSeconds,   // uint32 seconds → hours
  IdentityU16,        // raw uint16 (manufacturer/model)
  IdentityU32,        // raw uint32 (serial)
};

struct DseRegister {
  uint16_t address;
  DseDataWidth width;
  DseDecodeKind kind;
  bool isSigned;      // scaled path only
  float factor;       // engineering = raw * factor (Scaled)
  const char *key;
  const char *name;
  const char *unit;
  uint32_t spn;       // 0 if unused
};

struct DseDecodedValue {
  bool ok = false;
  bool invalid = false;  // GenComm sentinel (unavailable)
  int32_t raw = 0;
  uint32_t rawU = 0;
  float engineering = 0.0f;
  const DseRegister *reg = nullptr;
};

namespace DseRegisters {

size_t count();
const DseRegister &at(size_t index);
const DseRegister *findByKey(const char *key);
const DseRegister *findByAddress(uint16_t address);
const DseRegister *findByIndex1(size_t oneBased);

bool assembleRaw(const DseRegister &reg, const uint16_t *values,
                 uint8_t valueCount, int32_t *outRaw, uint32_t *outRawU);

DseDecodedValue decode(const DseRegister &reg, const uint16_t *values,
                       uint8_t valueCount);

void printList();
void printEntry(const DseRegister &reg);
void printDecoded(const DseDecodedValue &v);

void annotateRead(uint16_t address, uint16_t quantity, const uint16_t *values,
                  uint8_t valueCount);

void printAlarmActive(uint16_t address, uint16_t raw);

// Human-readable label for iterate output (e.g. "Line 1 voltage").
const char *friendlyLabel(const DseRegister &reg);

// Decode/print all known map entries covered by a batch FC03 reply.
void printBatchReadable(uint16_t batchStart, const uint16_t *values,
                        uint8_t valueCount);

// Alarm nibble helpers (register must be AlarmPacked).
const char *alarmSlotName(uint16_t address, uint8_t slot0to3);
const char *alarmStateName(uint8_t nibble);
const char *opModeName(uint16_t code);
const char *engStatusName(uint16_t code);

}  // namespace DseRegisters
