#include "devices/dse_scf.h"

#include <string.h>

#include <Arduino.h>

#include "modbus/modbus_client.h"

namespace {

// Published GenComm SCF keys (Senquip APN0029 / DSE 056-188 / Victron dse.py).
const DseScfCommand kCmds[] = {
    {"stop", 35700, "Select Stop mode"},
    {"auto", 35701, "Select Auto mode"},
    {"manual", 35702, "Select Manual mode"},
    {"test_on_load", 35703, "Select Test on Load mode (DG on load)"},
    {"auto_restore", 35704, "Select Auto with Manual Restore mode"},
    {"start_engine", 35705, "Start engine (Manual or Test modes)"},
    {"mute_alarm", 35706, "Mute alarm"},
    {"reset_alarms", 35707, "Reset alarms"},
    {"transfer_gen", 35708, "Transfer to generator"},
    {"transfer_mains", 35709, "Transfer to mains"},
    {"telemetry_start", 35732, "Telemetry start (if in Auto)"},
    {"telemetry_stop", 35733, "Cancel telemetry start (if in Auto)"},
    {"reset_alarms2", 35734, "Reset alarms (alt key)"},
    {"clear_telem_alarm", 35735, "Clear telemetry alarm flag"},
    {"lock_controls", 35736, "Lock user controls"},
    {"unlock_controls", 35737, "Unlock user controls"},
    {"reset_maint", 35738, "Reset maintenance alarm 1 due times"},
};

constexpr size_t kCmdCount = sizeof(kCmds) / sizeof(kCmds[0]);

}  // namespace

namespace DseScf {

size_t count() { return kCmdCount; }

const DseScfCommand &at(size_t index) { return kCmds[index]; }

const DseScfCommand *findByKey(const char *key) {
  if (!key || !*key) {
    return nullptr;
  }
  for (size_t i = 0; i < kCmdCount; i++) {
    if (strcasecmp(kCmds[i].key, key) == 0) {
      return &kCmds[i];
    }
  }
  return nullptr;
}

void printList() {
  Serial.println(F("## SCF commands (FC16 write key+~key @4104/4105)"));
  for (size_t i = 0; i < kCmdCount; i++) {
    Serial.print(F("  "));
    Serial.print(kCmds[i].key);
    Serial.print(F("  key="));
    Serial.print(kCmds[i].code);
    Serial.print(F("  ~"));
    Serial.print(static_cast<uint16_t>(65535u - kCmds[i].code));
    Serial.print(F("  "));
    Serial.println(kCmds[i].name);
  }
  Serial.println(F("Usage: scf <name> | scf list | scf flags | scf raw <key>"));
}

bool isSupported(uint16_t scfKey, const uint16_t flags[kFlagRegCount]) {
  if (!flags || scfKey < kKeyBase) {
    return false;
  }
  const uint16_t functionCode = static_cast<uint16_t>(scfKey - kKeyBase);
  const uint8_t idx = static_cast<uint8_t>(functionCode / 16);
  if (idx >= kFlagRegCount) {
    return false;
  }
  const uint8_t bitPos = static_cast<uint8_t>(15 - (functionCode % 16));
  return ((flags[idx] >> bitPos) & 0x1u) != 0;
}

ModbusResponse writeKey(uint8_t slaveId, uint16_t scfKey,
                        const ModbusTxnInfo *info) {
  if (scfKey == kKeyStop) {
    ModbusResponse blocked;
    blocked.result = ModbusResult::InvalidArgs;
    Serial.println(F("BLOCKED: SCF stop (35700) is disabled — DG must keep running."));
    return blocked;
  }
  const uint16_t values[2] = {scfKey,
                              static_cast<uint16_t>(65535u - scfKey)};
  return modbus.writeMultipleRegisters(slaveId, kRegKey, values, 2, info);
}

}  // namespace DseScf
