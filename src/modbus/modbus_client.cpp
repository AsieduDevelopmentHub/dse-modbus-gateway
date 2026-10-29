#include "modbus/modbus_client.h"

#include <string.h>

#include "config/config.h"
#include "modbus/modbus_crc.h"
#include "rs485/rs485.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "soc/uart_struct.h"
#endif

namespace {

struct RxByteTiming {
  uint16_t gapUs[64];
  uint8_t count;
  uint32_t charUs;
  uint8_t uartErr;
};

RxByteTiming gRxTiming;

uint32_t charTimeUs() {
  uint32_t baud = rs485.baud();
  if (baud < 300) {
    baud = 9600;
  }
  return (11UL * 1000000UL + baud - 1) / baud;
}

#if defined(ARDUINO_ARCH_ESP32)
uint8_t readUartErr() {
  uint8_t err = 0;
  if (UART1.int_raw.frm_err) {
    err |= 1;
  }
  if (UART1.int_raw.parity_err) {
    err |= 2;
  }
  if (UART1.int_raw.rxfifo_ovf) {
    err |= 4;
  }
  if (UART1.int_raw.glitch_det) {
    err |= 8;
  }
  if (UART1.int_raw.brk_det) {
    err |= 16;
  }
  UART1.int_clr.frm_err = 1;
  UART1.int_clr.parity_err = 1;
  UART1.int_clr.rxfifo_ovf = 1;
  UART1.int_clr.glitch_det = 1;
  UART1.int_clr.brk_det = 1;
  return err;
}
#else
uint8_t readUartErr() { return 0; }
#endif

void printRxGaps() {
  Serial.print(F("RX char us: "));
  Serial.println(gRxTiming.charUs);
  Serial.print(F("RX gaps us:"));
  if (gRxTiming.count == 0) {
    Serial.println(F(" (none)"));
    return;
  }
  uint8_t onGrid = 0;
  uint8_t late = 0;
  for (uint8_t i = 0; i < gRxTiming.count; i++) {
    Serial.print(' ');
    Serial.print(gRxTiming.gapUs[i]);
    if (i == 0) {
      continue;
    }
    const uint16_t gap = gRxTiming.gapUs[i];
    if (gap + 400 > gRxTiming.charUs && gap < gRxTiming.charUs + 500) {
      onGrid++;
    } else if (gap > gRxTiming.charUs + 500) {
      late++;
    }
  }
  Serial.println();
  Serial.print(F("RX timing: "));
  Serial.print(onGrid);
  Serial.print(F(" on 9600 grid, "));
  Serial.print(late);
  Serial.println(F(" late"));
  Serial.print(F("UART err:"));
  if (gRxTiming.uartErr == 0) {
    Serial.println(F(" none"));
  } else {
    if (gRxTiming.uartErr & 1) {
      Serial.print(F(" FRAMING"));
    }
    if (gRxTiming.uartErr & 2) {
      Serial.print(F(" PARITY"));
    }
    if (gRxTiming.uartErr & 4) {
      Serial.print(F(" OVERRUN"));
    }
    if (gRxTiming.uartErr & 8) {
      Serial.print(F(" GLITCH"));
    }
    if (gRxTiming.uartErr & 16) {
      Serial.print(F(" BREAK"));
    }
    Serial.println();
  }
}

// Length of one Modbus RTU ADU inside a capture. A fixed 5-byte slice
// of a normal FC03 reply (byte count + data + CRC) is not a frame.
size_t modbusAduLen(const uint8_t *rx, size_t n) {
  if (!rx || n < 5) {
    return n;
  }
  const uint8_t fc = rx[1];
  if ((fc & 0x80) != 0) {
    return 5;
  }
  if (fc == 0x03 || fc == 0x04) {
    const size_t need = static_cast<size_t>(3u + rx[2] + 2u);
    if (need >= 5 && need <= n) {
      return need;
    }
  }
  if ((fc == 0x06 || fc == 0x10) && n >= 8) {
    return 8;
  }
  return n;
}

}  // namespace

ModbusClient modbus;

void ModbusClient::begin() {}

void ModbusClient::setDiagnostic(bool enabled) { _diagnostic = enabled; }

void ModbusClient::dlog(const char *msg) {
  if (_diagnostic) {
    Serial.print(F("[MODBUS] "));
    Serial.println(msg);
  }
}

void ModbusClient::printHexLine(const char *label, const uint8_t *data,
                                size_t len) {
  if (label && label[0] != '\0') {
    Serial.print(label);
    Serial.println(":");
  }
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 < len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

void ModbusClient::printCrcCompare(const uint8_t *frame, size_t length) {
  printCrcAudit(frame, length);
}

void ModbusClient::printCrcAudit(const uint8_t *frame, size_t length) {
  if (length < 4) {
    Serial.println(F("PAYLOAD: (too short)"));
    Serial.println(F("CRC RESULT: FAIL"));
    return;
  }

  Serial.print(F("RAW RX: "));
  printHexLine("", frame, length);

  Serial.print(F("PAYLOAD: "));
  printHexLine("", frame, length - 2);

  const uint8_t crcLo = frame[length - 2];
  const uint8_t crcHi = frame[length - 1];
  const uint16_t received =
      static_cast<uint16_t>(crcLo) | (static_cast<uint16_t>(crcHi) << 8);
  const uint16_t calculated = modbusCrc16(frame, length - 2);

  Serial.print(F("RECEIVED CRC LOW:  0x"));
  if (crcLo < 0x10) {
    Serial.print('0');
  }
  Serial.println(crcLo, HEX);
  Serial.print(F("RECEIVED CRC HIGH: 0x"));
  if (crcHi < 0x10) {
    Serial.print('0');
  }
  Serial.println(crcHi, HEX);
  Serial.print(F("RECEIVED CRC COMBINED: 0x"));
  Serial.println(received, HEX);
  Serial.print(F("CALCULATED CRC: 0x"));
  Serial.println(calculated, HEX);
  Serial.print(F("CRC RESULT: "));
  Serial.println(received == calculated ? F("PASS") : F("FAIL"));
  Serial.println(F("(combine rule: low | (high << 8); CRC bytes excluded from calc)"));
}

const char *ModbusClient::exceptionName(uint8_t code) {
  switch (code) {
    case 0x01:
      return "Illegal Function";
    case 0x02:
      return "Illegal Data Address";
    case 0x03:
      return "Illegal Data Value";
    case 0x04:
      return "Slave Device Failure";
    case 0x05:
      return "Acknowledge";
    case 0x06:
      return "Slave Device Busy";
    case 0x07:
      return "Illegal Value";
    case 0x08:
      return "Memory Parity Error";
    case 0x0A:
      return "Gateway Path Unavailable";
    case 0x0B:
      return "Gateway Target Failed to Respond";
    default:
      return "Unknown Exception";
  }
}

const char *ModbusClient::resultName(ModbusResult r) {
  switch (r) {
    case ModbusResult::Ok:
      return "VALID MODBUS RESPONSE";
    case ModbusResult::Timeout:
      return "MODBUS TIMEOUT";
    case ModbusResult::CrcError:
      return "INVALID MODBUS FRAME (CRC)";
    case ModbusResult::Exception:
      return "VALID MODBUS EXCEPTION";
    case ModbusResult::FrameError:
      return "INVALID MODBUS FRAME";
    case ModbusResult::InvalidArgs:
      return "INVALID ARGS";
    case ModbusResult::UnexpectedFunction:
      return "UNEXPECTED FUNCTION RESPONSE";
    case ModbusResult::Partial:
      return "MODBUS PARTIAL RESPONSE";
    default:
      return "UNKNOWN";
  }
}

void ModbusClient::printTimeoutDetail(bool txDone, bool deToRx, size_t rxLen,
                                      uint32_t elapsedMs) {
  Serial.println(F("MODBUS TIMEOUT"));
  Serial.println(F("TX sent: YES"));
  Serial.print(F("TX completed: "));
  Serial.println(txDone ? F("YES") : F("NO"));
  Serial.print(F("DE/RE switched to RX: "));
  Serial.println(deToRx ? F("YES") : F("NO"));
  Serial.print(F("RX bytes received: "));
  Serial.println(rxLen);
  Serial.print(F("Elapsed: "));
  Serial.print(elapsedMs);
  Serial.println(F(" ms"));
}

bool ModbusClient::runCrcSelfTest() {
  Serial.println();
  Serial.println(F("## CRC SELF TEST"));
  uint8_t fc03[] = {0x32, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  const bool pass03 = (modbusAppendCrc(fc03, 6) == 8 && fc03[6] == 0x81 &&
                       fc03[7] == 0xC9 && modbusCrcOk(fc03, 8));
  uint8_t fc04[] = {0x32, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  const bool pass04 = (modbusAppendCrc(fc04, 6) == 8 && fc04[6] == 0x34 &&
                       fc04[7] == 0x09 && modbusCrcOk(fc04, 8));
  const uint8_t exc[] = {0x32, 0x84, 0x02, 0x32, 0xCE};
  const bool passExc = modbusCrcOk(exc, 5);

  Serial.print(F("FC03 request: "));
  Serial.println(pass03 ? F("PASS") : F("FAIL"));
  Serial.print(F("FC04 request: "));
  Serial.println(pass04 ? F("PASS") : F("FAIL"));
  Serial.print(F("FC04 exception frame: "));
  Serial.println(passExc ? F("PASS") : F("FAIL"));
  Serial.print(F("Overall: "));
  Serial.println((pass03 && pass04 && passExc) ? F("PASS") : F("FAIL"));
  return pass03 && pass04 && passExc;
}

size_t ModbusClient::collectRx(uint8_t *buf, size_t bufMax, uint32_t timeoutMs,
                               uint32_t *outFirstByteMs) {
  const uint32_t charUs = charTimeUs();
  // Software gaps are dequeue time, not wire time. The UART can release the
  // CRC a few milliseconds after the header. Only a long pause is a new burst.
  const uint32_t splitUs = charUs * 8;
  const uint32_t eofUs = charUs * 8;
  gRxTiming.charUs = charUs;
  gRxTiming.count = 0;
  gRxTiming.uartErr = 0;
  (void)readUartErr();
  if (outFirstByteMs) {
    *outFirstByteMs = 0;
  }
  if (!buf || bufMax == 0) {
    return 0;
  }

  // FC03 qty 32 is 69 bytes. A 64-byte cap truncated the CRC and
  // made the DSE instrumentation batch fail.
  uint8_t raw[96];
  uint16_t gaps[96];
  const size_t rawMax = sizeof(raw) < bufMax ? sizeof(raw) : bufMax;
  size_t n = 0;
  const uint32_t startMs = millis();
  uint32_t lastByteUs = 0;
  bool saw = false;
  uint32_t spins = 0;

  while ((millis() - startMs) < timeoutMs && n < rawMax) {
    while (rs485.stream().available() && n < rawMax) {
      const uint32_t nowUs = micros();
      uint32_t gap = 0;
      if (!saw) {
        gap = (millis() - startMs) * 1000UL;
        if (outFirstByteMs) {
          *outFirstByteMs = millis() - startMs;
        }
      } else {
        gap = nowUs - lastByteUs;
      }
      lastByteUs = nowUs;
      saw = true;
      gaps[n] = gap > 65535UL ? 65535 : static_cast<uint16_t>(gap);
      raw[n++] = static_cast<uint8_t>(rs485.stream().read());
    }
    if (saw && (micros() - lastByteUs) >= eofUs) {
      // A 1-byte DE glitch is not the frame. Keep waiting for the reply.
      if (n >= 3) {
        break;
      }
    }
    if ((++spins & 31u) == 0) {
      yield();
    } else {
      delayMicroseconds(30);
    }
  }

  size_t begin = 0;
  for (size_t i = 1; i < n; i++) {
    if (gaps[i] >= splitUs && (i - begin) < 4 && (n - i) > (i - begin)) {
      begin = i;
    }
  }
  size_t end = n;
  for (size_t i = begin + 1; i < n; i++) {
    if (gaps[i] >= splitUs) {
      end = i;
      break;
    }
  }

  const size_t len = end - begin;
  memcpy(buf, raw + begin, len);
  gRxTiming.count = static_cast<uint8_t>(len > 64 ? 64 : len);
  for (uint8_t i = 0; i < gRxTiming.count; i++) {
    gRxTiming.gapUs[i] = gaps[begin + i];
  }
  gRxTiming.uartErr = readUartErr();
  return len;
}

size_t ModbusClient::exchange(uint8_t slaveId, uint8_t functionCode,
                               uint16_t address, uint16_t quantity,
                               uint8_t *rx, size_t rxMax, uint32_t timeoutMs) {
  if (!rx || rxMax == 0 || quantity == 0 || quantity > 125) {
    return 0;
  }
  if (timeoutMs == 0) {
    timeoutMs = MODBUS_RESPONSE_TIMEOUT_MS;
  }
  uint8_t req[8];
  req[0] = slaveId;
  req[1] = functionCode;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((quantity >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(quantity & 0xFF);
  const size_t reqLen = modbusAppendCrc(req, 6);
  rs485.flushRx();
  rs485.transmit(req, reqLen, nullptr, nullptr, nullptr);
  (void)rs485.flushRxCounted(nullptr, 0);
  return collectRx(rx, rxMax, timeoutMs, nullptr);
}

RawRs485Capture ModbusClient::rawCapture(uint8_t slaveId, uint8_t functionCode,
                                         uint16_t address, uint16_t quantity) {
  RawRs485Capture cap;
  cap.txnId = nextTxnId();

  uint8_t req[8];
  req[0] = slaveId;
  req[1] = functionCode;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((quantity >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(quantity & 0xFF);
  cap.txLen = modbusAppendCrc(req, 6);
  memcpy(cap.tx, req, cap.txLen);

  Serial.println(F("========================================"));
  Serial.println(F("RS485 RAW CAPTURE (no Modbus parse)"));
  Serial.print(F("TXN #"));
  Serial.println(cap.txnId);
  Serial.print(F("DE/RE before TX: "));
  Serial.println(rs485.deReLevel());

  uint8_t stale[16];
  const size_t staleN = rs485.flushRxCounted(stale, sizeof(stale));
  Serial.print(F("RX stale bytes cleared: "));
  Serial.println(staleN);

  cap.deBefore = 1;
  cap.txWritten =
      rs485.transmit(cap.tx, cap.txLen, &cap.tTxStartUs, &cap.tTxCompleteUs,
                     &cap.tDeLowUs);
  cap.deAfter = rs485.deReLevel();
  (void)rs485.flushRxCounted(nullptr, 0);  // drop TX echo only

  Serial.print(F("TX bytes written: "));
  Serial.print(cap.txWritten);
  Serial.print(F(" / "));
  Serial.println(cap.txLen);
  Serial.print(F("TX: "));
  printHexLine("", cap.tx, cap.txLen);
  Serial.print(F("DE/RE after TX->RX: "));
  Serial.println(cap.deAfter);
  Serial.print(F("TX start us: "));
  Serial.println(cap.tTxStartUs);
  Serial.print(F("TX complete us: "));
  Serial.println(cap.tTxCompleteUs);
  Serial.print(F("DE->RX us: "));
  Serial.println(cap.tDeLowUs);

  const uint32_t t0 = millis();
  uint32_t firstMs = 0;
  cap.rxLen = collectRx(cap.rx, sizeof(cap.rx), MODBUS_RESPONSE_TIMEOUT_MS,
                        &firstMs);
  cap.tFirstRxMs = firstMs;
  cap.elapsedMs = millis() - t0;

  Serial.print(F("RX: "));
  if (cap.rxLen == 0) {
    Serial.println(F("(none)"));
  } else {
    printHexLine("", cap.rx, cap.rxLen);
  }
  Serial.print(F("RX LENGTH: "));
  Serial.println(cap.rxLen);
  printRxGaps();
  Serial.print(F("Time to first RX byte: "));
  Serial.print(cap.tFirstRxMs);
  Serial.println(F(" ms"));
  Serial.println(F("========================================"));
  Serial.println(F("CRC audit of captured RX:"));
  if (cap.rxLen > 0) {
    const size_t adu = modbusAduLen(cap.rx, cap.rxLen);
    printCrcAudit(cap.rx, adu);
    if (adu < cap.rxLen) {
      Serial.println(F("NOTE: extra bytes after the Modbus frame — full buffer:"));
      printCrcAudit(cap.rx, cap.rxLen);
    }
  }
  return cap;
}

ModbusResponse ModbusClient::readHoldingRegisters(uint8_t slaveId,
                                                  uint16_t address,
                                                  uint16_t quantity,
                                                  const ModbusTxnInfo *info) {
  return readRegisters(slaveId, 0x03, address, quantity, info);
}

ModbusResponse ModbusClient::readInputRegisters(uint8_t slaveId,
                                                uint16_t address,
                                                uint16_t quantity,
                                                const ModbusTxnInfo *info) {
  return readRegisters(slaveId, 0x04, address, quantity, info);
}

ModbusResponse ModbusClient::readRegisters(uint8_t slaveId,
                                           uint8_t functionCode,
                                           uint16_t address,
                                           uint16_t quantity,
                                           const ModbusTxnInfo *info) {
  ModbusResponse empty;
  const bool quiet = info && info->quiet;
  if (quantity == 0 || quantity > 32) {
    empty.result = ModbusResult::InvalidArgs;
    Serial.println(F("ERROR: quantity must be 1..32"));
    return empty;
  }
  if (functionCode != 0x03 && functionCode != 0x04) {
    empty.result = ModbusResult::InvalidArgs;
    Serial.println(F("ERROR: supports FC 0x03 and 0x04 only"));
    return empty;
  }

  const uint32_t txn = nextTxnId();
  if (!quiet) {
    Serial.println(F("========================================"));
    Serial.println(F("MODBUS RTU DIAGNOSTIC"));
    Serial.print(F("TXN #"));
    Serial.println(txn);
    if (info && info->deviceName) {
      Serial.print(F("Device: "));
      Serial.println(info->deviceName);
    }
    Serial.print(F("REQUEST fields: slave="));
    Serial.print(slaveId);
    Serial.print(F(" fc=0x"));
    Serial.print(functionCode, HEX);
    Serial.print(F(" addr=0x"));
    Serial.print(address, HEX);
    Serial.print(F(" qty="));
    Serial.println(quantity);
    if (info && info->baud) {
      Serial.print(F("Runtime UART: "));
      Serial.print(info->baud);
      Serial.print(F(" "));
      Serial.print(info->dataBits);
      Serial.print(info->parity == 1 ? 'O' : (info->parity == 2 ? 'E' : 'N'));
      Serial.println(info->stopBits);
    }
  }

  uint8_t req[8];
  req[0] = slaveId;
  req[1] = functionCode;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((quantity >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(quantity & 0xFF);
  const size_t reqLen = modbusAppendCrc(req, 6);

  if (!quiet) {
    Serial.print(F("FINAL TX BYTE ARRAY: "));
    printHexLine("", req, reqLen);
  }

  ModbusResponse resp =
      transact(req, reqLen, slaveId, functionCode, quantity, info, false);
  resp.txnId = txn;
  return resp;
}

ModbusResponse ModbusClient::writeMultipleRegisters(
    uint8_t slaveId, uint16_t address, const uint16_t *values,
    uint16_t quantity, const ModbusTxnInfo *info) {
  ModbusResponse empty;
  if (!values || quantity == 0 || quantity > 16) {
    empty.result = ModbusResult::InvalidArgs;
    Serial.println(F("ERROR: write quantity must be 1..16"));
    return empty;
  }

  const bool quiet = info && info->quiet;
  const uint32_t txn = nextTxnId();
  if (!quiet) {
    Serial.println(F("========================================"));
    Serial.println(F("MODBUS RTU WRITE (FC16)"));
    Serial.print(F("TXN #"));
    Serial.println(txn);
    if (info && info->deviceName) {
      Serial.print(F("Device: "));
      Serial.println(info->deviceName);
    }
    Serial.print(F("REQUEST: slave="));
    Serial.print(slaveId);
    Serial.print(F(" addr="));
    Serial.print(address);
    Serial.print(F(" qty="));
    Serial.println(quantity);
  }

  // slave FC addr_hi addr_lo qty_hi qty_lo byte_count data... crc
  uint8_t req[64];
  const uint8_t byteCount = static_cast<uint8_t>(quantity * 2);
  req[0] = slaveId;
  req[1] = 0x10;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((quantity >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(quantity & 0xFF);
  req[6] = byteCount;
  for (uint16_t i = 0; i < quantity; i++) {
    req[7 + i * 2] = static_cast<uint8_t>((values[i] >> 8) & 0xFF);
    req[8 + i * 2] = static_cast<uint8_t>(values[i] & 0xFF);
  }
  const size_t payloadLen = static_cast<size_t>(7 + byteCount);
  const size_t reqLen = modbusAppendCrc(req, payloadLen);

  if (!quiet) {
    Serial.print(F("FINAL TX BYTE ARRAY: "));
    printHexLine("", req, reqLen);
  }

  ModbusResponse resp =
      transact(req, reqLen, slaveId, 0x10, quantity, info, true);
  resp.txnId = txn;
  return resp;
}

ModbusResponse ModbusClient::writeSingleRegister(uint8_t slaveId,
                                                 uint16_t address,
                                                 uint16_t value,
                                                 const ModbusTxnInfo *info) {
  const uint32_t txn = nextTxnId();
  Serial.println(F("========================================"));
  Serial.println(F("MODBUS RTU WRITE (FC06)"));
  Serial.print(F("TXN #"));
  Serial.println(txn);
  Serial.print(F("REQUEST: slave="));
  Serial.print(slaveId);
  Serial.print(F(" addr="));
  Serial.print(address);
  Serial.print(F(" value="));
  Serial.println(value);

  uint8_t req[8];
  req[0] = slaveId;
  req[1] = 0x06;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((value >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(value & 0xFF);
  const size_t reqLen = modbusAppendCrc(req, 6);
  Serial.print(F("FINAL TX BYTE ARRAY: "));
  printHexLine("", req, reqLen);

  // FC06 echoes address in bytes 2-3 and value in bytes 4-5. Reuse the
  // FC16 matcher, which treats bytes 4-5 as the echoed quantity: pass the
  // value so a correct echo matches.
  ModbusResponse resp =
      transact(req, reqLen, slaveId, 0x06, value, info, true);
  resp.txnId = txn;
  return resp;
}

ModbusResponse ModbusClient::transact(const uint8_t *request, size_t requestLen,
                                      uint8_t expectedSlave, uint8_t expectedFc,
                                      uint16_t quantity,
                                      const ModbusTxnInfo *info,
                                      bool writeResponse) {
  ModbusResponse resp;
  const bool quiet = info && info->quiet;
  const uint8_t expectedExcFc = static_cast<uint8_t>(expectedFc | 0x80);

  if (!quiet) {
    Serial.println(F("[1] Clear RX buffer"));
  }
  uint8_t stale[16];
  const size_t staleN = rs485.flushRxCounted(stale, sizeof(stale));
  if (!quiet) {
    Serial.print(F("RX stale bytes: "));
    Serial.println(staleN);
    if (staleN > 0) {
      printHexLine("STALE", stale,
                   staleN > sizeof(stale) ? sizeof(stale) : staleN);
    }
    Serial.print(F("[2] DE/RE before TX: "));
    Serial.println(rs485.deReLevel());
    Serial.println(F("[3] TX + hardware TX-done wait"));
  }

  uint32_t tStart = 0, tDone = 0, tDeLow = 0;
  const size_t written =
      rs485.transmit(request, requestLen, &tStart, &tDone, &tDeLow);
  if (!quiet) {
    Serial.print(F("write() returned: "));
    Serial.print(written);
    Serial.print(F(" expected: "));
    Serial.println(requestLen);
    Serial.print(F("DE/RE after switch to RX: "));
    Serial.println(rs485.deReLevel());
    Serial.print(F("TX start us: "));
    Serial.println(tStart);
    Serial.print(F("TX complete us: "));
    Serial.println(tDone);
    Serial.print(F("DE->RX us: "));
    Serial.println(tDeLow);
    Serial.print(F("TX settle us (config): "));
    Serial.println(rs485.txSettleUs());
  }

  // With DE+/RE tied (THVD1450), receiver was off during TX — usually no echo.
  // Flush only residual noise before the slave's inter-frame gap ends.
  const size_t echoN = rs485.flushRxCounted(nullptr, 0);
  if (!quiet && echoN) {
    Serial.print(F("TX-echo bytes dropped: "));
    Serial.println(echoN);
  }

  if (!quiet) {
    Serial.println(F("[4] Waiting for response..."));
  }
  uint32_t firstMs = 0;
  const uint32_t tWait = millis();
  const size_t got =
      collectRx(resp.rawRx, sizeof(resp.rawRx), MODBUS_RESPONSE_TIMEOUT_MS,
                &firstMs);
  resp.rawRxLen = got;
  const uint32_t elapsed = millis() - tWait;

  if (!quiet) {
    Serial.println(F("[5] RX"));
    if (got == 0) {
      Serial.println(F("(none)"));
      printTimeoutDetail(written == requestLen, true, 0, elapsed);
      resp.result = ModbusResult::Timeout;
      Serial.print(F("RESULT: "));
      Serial.println(resultName(resp.result));
      Serial.println(F("========================================"));
      return resp;
    }
    printHexLine("", resp.rawRx, got);
    Serial.print(F("RX length: "));
    Serial.println(got);
    printRxGaps();
    Serial.print(F("Time to first RX byte: "));
    Serial.print(firstMs);
    Serial.println(F(" ms"));
  } else if (got == 0) {
    resp.result = ModbusResult::Timeout;
    return resp;
  }

  if (got < 5) {
    if (!quiet) {
      Serial.println(F("RESULT: MODBUS PARTIAL RESPONSE"));
      Serial.println(F("========================================"));
    }
    resp.result = ModbusResult::Partial;
    return resp;
  }

  if (!quiet) {
    Serial.println(F("[6] Frame validation / CRC audit"));
  }

  size_t frameOff = 0;
  size_t frameLen = 0;
  enum class Found : uint8_t { None, Ok, Exception, UnexpectedFc };
  Found found = Found::None;

  for (size_t off = 0; off < got; off++) {
    if (resp.rawRx[off] != expectedSlave) {
      continue;
    }
    const size_t remain = got - off;
    if (remain < 5) {
      break;
    }
    if (resp.rawRx[off + 1] == expectedExcFc &&
        modbusCrcOk(resp.rawRx + off, 5)) {
      frameOff = off;
      frameLen = 5;
      found = Found::Exception;
      break;
    }
    if ((resp.rawRx[off + 1] & 0x80) && modbusCrcOk(resp.rawRx + off, 5)) {
      frameOff = off;
      frameLen = 5;
      found = Found::UnexpectedFc;
      break;
    }
    if (resp.rawRx[off + 1] == expectedFc) {
      if (writeResponse) {
        constexpr size_t need = 8;
        if (remain >= need && modbusCrcOk(resp.rawRx + off, need)) {
          const uint16_t echoQty = static_cast<uint16_t>(
              (resp.rawRx[off + 4] << 8) | resp.rawRx[off + 5]);
          if (echoQty == quantity) {
            frameOff = off;
            frameLen = need;
            found = Found::Ok;
            break;
          }
        }
      } else {
        const uint8_t bc = resp.rawRx[off + 2];
        const size_t need = static_cast<size_t>(3 + bc + 2);
        if (remain >= need && bc == (quantity * 2) &&
            modbusCrcOk(resp.rawRx + off, need)) {
          frameOff = off;
          frameLen = need;
          found = Found::Ok;
          break;
        }
      }
    }
  }

  if (found == Found::None) {
    if (!quiet) {
      printCrcAudit(resp.rawRx, modbusAduLen(resp.rawRx, got));
      Serial.print(F("Slave byte: "));
      Serial.println(resp.rawRx[0]);
      Serial.print(F("Function byte: 0x"));
      Serial.println(resp.rawRx[1], HEX);
      Serial.print(F("Expected exception FC: 0x"));
      Serial.println(expectedExcFc, HEX);
      if ((resp.rawRx[1] & 0x80) && resp.rawRx[1] != expectedExcFc) {
        Serial.println(F("UNEXPECTED FUNCTION (and CRC FAIL)"));
      }
      Serial.println(F("RESULT: INVALID MODBUS FRAME (CRC)"));
      Serial.println(F("NOTE: Do NOT treat this as a valid exception."));
      Serial.println(F("========================================"));
    }
    resp.result = ModbusResult::CrcError;
    return resp;
  }

  if (frameLen > sizeof(resp.rawRx)) {
    resp.result = ModbusResult::FrameError;
    return resp;
  }
  uint8_t frame[96];
  memcpy(frame, resp.rawRx + frameOff, frameLen);
  if (!quiet) {
    printCrcAudit(frame, frameLen);
  }
  resp.slaveId = frame[0];
  resp.functionCode = frame[1];

  if (found == Found::UnexpectedFc) {
    resp.exceptionCode = frame[2];
    resp.result = ModbusResult::UnexpectedFunction;
    if (!quiet) {
      Serial.println(F("RESULT: UNEXPECTED FUNCTION RESPONSE"));
      Serial.println(F("========================================"));
    }
    return resp;
  }

  if (found == Found::Exception) {
    resp.exceptionCode = frame[2];
    resp.result = ModbusResult::Exception;
    if (!quiet) {
      Serial.print(F("Exception 0x"));
      Serial.print(resp.exceptionCode, HEX);
      Serial.print(F(" "));
      Serial.println(exceptionName(resp.exceptionCode));
      Serial.println(F("RESULT: VALID MODBUS EXCEPTION"));
      Serial.println(F("========================================"));
    }
    return resp;
  }

  if (writeResponse) {
    resp.byteCount = 0;
    resp.valueCount = 0;
    resp.values[0] =
        static_cast<uint16_t>((frame[2] << 8) | frame[3]);
    resp.values[1] =
        static_cast<uint16_t>((frame[4] << 8) | frame[5]);
    if (!quiet) {
      Serial.print(F("WRITE ACK addr="));
      Serial.print(resp.values[0]);
      Serial.print(F(" qty="));
      Serial.println(resp.values[1]);
      Serial.println(F("RESULT: VALID MODBUS WRITE RESPONSE"));
      Serial.println(F("========================================"));
    }
    resp.result = ModbusResult::Ok;
    return resp;
  }

  resp.byteCount = frame[2];
  resp.valueCount = static_cast<uint8_t>(resp.byteCount / 2);
  for (uint8_t i = 0; i < resp.valueCount; i++) {
    resp.values[i] = static_cast<uint16_t>((frame[3 + i * 2] << 8) |
                                           frame[4 + i * 2]);
    if (!quiet) {
      Serial.print(F("Register["));
      Serial.print(i);
      Serial.print(F("]=0x"));
      Serial.println(resp.values[i], HEX);
    }
  }
  resp.result = ModbusResult::Ok;
  if (!quiet) {
    Serial.println(F("RESULT: VALID MODBUS RESPONSE"));
    Serial.println(F("========================================"));
  }
  return resp;
}
