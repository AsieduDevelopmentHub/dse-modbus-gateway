#include "devices/dse7320_registers.h"
#include "devices/dse_scf.h"

#include <Arduino.h>

// GATEWAY_LITE: AVR 5V isolation benches keep Modbus CLI + CRC/raw diagnostics
// only. Full DSE named map / SCF tables stay on ESP32 (string tables blow AVR RAM).

#if defined(GATEWAY_LITE)

namespace {

void liteNotice() {
  Serial.println(
      F("GATEWAY_LITE: DSE named map/SCF omitted (AVR RAM). Use rawread/rs485raw."));
}

static const DseRegister kDummy = {
    0, DseDataWidth::Int16, DseDecodeKind::Scaled, false, 1.0f, "", "", "", 0};

static const DseScfCommand kScfDummy = {"", 0, ""};

}  // namespace

namespace DseRegisters {

size_t count() { return 0; }
const DseRegister &at(size_t) { return kDummy; }
const DseRegister *findByKey(const char *) { return nullptr; }
const DseRegister *findByAddress(uint16_t) { return nullptr; }
const DseRegister *findByIndex1(size_t) { return nullptr; }

bool assembleRaw(const DseRegister &, const uint16_t *, uint8_t, int32_t *,
                 uint32_t *) {
  return false;
}

DseDecodedValue decode(const DseRegister &, const uint16_t *, uint8_t) {
  return {};
}

void printList() { liteNotice(); }
void printEntry(const DseRegister &) { liteNotice(); }
void printDecoded(const DseDecodedValue &) { liteNotice(); }
void annotateRead(uint16_t, uint16_t, const uint16_t *, uint8_t) {}
void printAlarmActive(uint16_t, uint16_t) { liteNotice(); }
const char *friendlyLabel(const DseRegister &) { return ""; }
void printBatchReadable(uint16_t, const uint16_t *, uint8_t) { liteNotice(); }
const char *alarmSlotName(uint16_t, uint8_t) { return ""; }
const char *alarmStateName(uint8_t) { return ""; }
const char *opModeName(uint16_t) { return ""; }
const char *engStatusName(uint16_t) { return ""; }

}  // namespace DseRegisters

namespace DseScf {

size_t count() { return 0; }
const DseScfCommand &at(size_t) { return kScfDummy; }
const DseScfCommand *findByKey(const char *) { return nullptr; }
void printList() { liteNotice(); }
bool isSupported(uint16_t, const uint16_t[kFlagRegCount]) { return false; }

ModbusResponse writeKey(uint8_t, uint16_t, const ModbusTxnInfo *) {
  liteNotice();
  ModbusResponse r;
  r.result = ModbusResult::InvalidArgs;
  return r;
}

}  // namespace DseScf

#endif  // GATEWAY_LITE
