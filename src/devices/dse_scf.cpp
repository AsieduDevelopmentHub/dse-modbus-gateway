#include "devices/dse_scf.h"

#if !defined(GATEWAY_LITE)

#include <string.h>

#include <Arduino.h>
#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif

#include "modbus/modbus_client.h"
#include "util/str_util.h"

namespace {

struct DseScfCommandFlash {
  PGM_P key;
  uint16_t code;
  PGM_P name;
};

// Published DSE SCF keys (Senquip APN0029 / DSE 056-188 / Victron dse.py).
// Named PROGMEM strings (Arduino PSTR() cannot appear in static initializers)
static const char scf_ps_9df22f196a[] PROGMEM = "auto";
static const char scf_ps_c0442674cc[] PROGMEM = "Select Auto mode";
static const char scf_ps_3c78b35502[] PROGMEM = "manual";
static const char scf_ps_05298bfaae[] PROGMEM = "Select Manual mode";
static const char scf_ps_1e07488ed4[] PROGMEM = "test_on_load";
static const char scf_ps_afd3c0c267[] PROGMEM = "Select Test on Load mode (DG on load)";
static const char scf_ps_b4ba9ff910[] PROGMEM = "auto_restore";
static const char scf_ps_76c6ce5950[] PROGMEM = "Select Auto with Manual Restore mode";
static const char scf_ps_9116b0b8d0[] PROGMEM = "start_engine";
static const char scf_ps_6f03aa4701[] PROGMEM = "Start engine (Manual or Test modes)";
static const char scf_ps_e8df01bfd2[] PROGMEM = "mute_alarm";
static const char scf_ps_89e0cba23b[] PROGMEM = "Mute alarm";
static const char scf_ps_131fd1a494[] PROGMEM = "reset_alarms";
static const char scf_ps_dde6372401[] PROGMEM = "Reset alarms";
static const char scf_ps_301b496b37[] PROGMEM = "transfer_gen";
static const char scf_ps_4c268346b2[] PROGMEM = "Transfer to generator";
static const char scf_ps_6d4fc4c9b0[] PROGMEM = "transfer_mains";
static const char scf_ps_cc4a75902e[] PROGMEM = "Transfer to mains";
static const char scf_ps_d3df436db7[] PROGMEM = "telemetry_start";
static const char scf_ps_adba04ce8a[] PROGMEM = "Telemetry start (if in Auto)";
static const char scf_ps_06ca5ea67d[] PROGMEM = "telemetry_stop";
static const char scf_ps_b0a0669e50[] PROGMEM = "Cancel telemetry start (if in Auto)";
static const char scf_ps_8c4c4dc0d7[] PROGMEM = "reset_alarms2";
static const char scf_ps_f2f26017a4[] PROGMEM = "Reset alarms (alt key)";
static const char scf_ps_18ad7d1e78[] PROGMEM = "clear_telem_alarm";
static const char scf_ps_28b51409f2[] PROGMEM = "Clear telemetry alarm flag";
static const char scf_ps_68ef9a0fe2[] PROGMEM = "lock_controls";
static const char scf_ps_e8388ca9bc[] PROGMEM = "Lock user controls";
static const char scf_ps_e52668f957[] PROGMEM = "unlock_controls";
static const char scf_ps_dba60e5cf8[] PROGMEM = "Unlock user controls";
static const char scf_ps_16dc8d16bf[] PROGMEM = "reset_maint";
static const char scf_ps_1dae47458b[] PROGMEM = "Reset maintenance alarm 1 due times";

const DseScfCommandFlash kCmds[] PROGMEM = {
    {scf_ps_9df22f196a, 35701, scf_ps_c0442674cc},
    {scf_ps_3c78b35502, 35702, scf_ps_05298bfaae},
    {scf_ps_1e07488ed4, 35703, scf_ps_afd3c0c267},
    {scf_ps_b4ba9ff910, 35704, scf_ps_76c6ce5950},
    {scf_ps_9116b0b8d0, 35705, scf_ps_6f03aa4701},
    {scf_ps_e8df01bfd2, 35706, scf_ps_89e0cba23b},
    {scf_ps_131fd1a494, 35707, scf_ps_dde6372401},
    {scf_ps_301b496b37, 35708, scf_ps_4c268346b2},
    {scf_ps_6d4fc4c9b0, 35709, scf_ps_cc4a75902e},
    {scf_ps_d3df436db7, 35732, scf_ps_adba04ce8a},
    {scf_ps_06ca5ea67d, 35733, scf_ps_b0a0669e50},
    {scf_ps_8c4c4dc0d7, 35734, scf_ps_f2f26017a4},
    {scf_ps_18ad7d1e78, 35735, scf_ps_28b51409f2},
    {scf_ps_68ef9a0fe2, 35736, scf_ps_e8388ca9bc},
    {scf_ps_e52668f957, 35737, scf_ps_dba60e5cf8},
    {scf_ps_16dc8d16bf, 35738, scf_ps_1dae47458b},
};

constexpr size_t kCmdCount = sizeof(kCmds) / sizeof(kCmds[0]);

void loadCmd(size_t index, DseScfCommand *out) {
  DseScfCommandFlash f;
  memcpy_P(&f, &kCmds[index], sizeof(f));
  out->code = f.code;
  strncpy_P(out->key, f.key, sizeof(out->key) - 1);
  out->key[sizeof(out->key) - 1] = '\0';
  strncpy_P(out->name, f.name, sizeof(out->name) - 1);
  out->name[sizeof(out->name) - 1] = '\0';
}

}  // namespace

namespace DseScf {

size_t count() { return kCmdCount; }

const DseScfCommand &at(size_t index) {
  static DseScfCommand buf;
  if (index >= kCmdCount) {
    index = 0;
  }
  loadCmd(index, &buf);
  return buf;
}

const DseScfCommand *findByKey(const char *key) {
  static DseScfCommand scratch;
  if (!key || !*key) {
    return nullptr;
  }
  for (size_t i = 0; i < kCmdCount; i++) {
    loadCmd(i, &scratch);
    if (strEqualsIgnoreCase(scratch.key, key) == 0) {
      return &scratch;
    }
  }
  return nullptr;
}

void printList() {
  Serial.println(F("## SCF commands (FC16 write key+~key @4104/4105)"));
  for (size_t i = 0; i < kCmdCount; i++) {
    DseScfCommand cmd;
    loadCmd(i, &cmd);
    Serial.print(F("  "));
    Serial.print(cmd.key);
    Serial.print(F("  key="));
    Serial.print(cmd.code);
    Serial.print(F("  ~"));
    Serial.print(static_cast<uint16_t>(65535u - cmd.code));
    Serial.print(F("  "));
    Serial.println(cmd.name);
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

#endif  // !GATEWAY_LITE
