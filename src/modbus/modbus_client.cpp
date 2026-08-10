#include "modbus/modbus_client.h"

#include <string.h>

#include "config/config.h"
#include "modbus/modbus_crc.h"
#include "rs485/rs485.h"

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
  if (length < 4) {
    Serial.println(F("CRC received: (frame too short)"));
    Serial.println(F("CRC calculated: (n/a)"));
    return;
  }
  const uint16_t received =
      static_cast<uint16_t>(frame[length - 2]) |
      (static_cast<uint16_t>(frame[length - 1]) << 8);
  const uint16_t calculated = modbusCrc16(frame, length - 2);

  Serial.print(F("CRC received:   0x"));
  if (received < 0x1000) {
    Serial.print('0');
  }
  if (received < 0x100) {
    Serial.print('0');
  }
  if (received < 0x10) {
    Serial.print('0');
  }
  Serial.println(received, HEX);
  Serial.print(F("CRC calculated: 0x"));
  if (calculated < 0x1000) {
    Serial.print('0');
  }
  if (calculated < 0x100) {
    Serial.print('0');
  }
  if (calculated < 0x10) {
    Serial.print('0');
  }
  Serial.println(calculated, HEX);
  Serial.print(F("CRC: "));
  Serial.println(received == calculated ? F("VALID") : F("INVALID"));
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
  Serial.print(F("TX sent: YES"));
  Serial.println();
  Serial.print(F("TX completed: "));
  Serial.println(txDone ? F("YES") : F("NO"));
  Serial.print(F("DE/RE switched to RX: "));
  Serial.println(deToRx ? F("YES") : F("NO"));
  Serial.print(F("RX bytes received: "));
  Serial.println(rxLen);
  Serial.print(F("Elapsed: "));
  Serial.print(elapsedMs);
  Serial.println(F(" ms"));
  Serial.println(F("Possible causes:"));
  Serial.println(F("- slave not responding"));
  Serial.println(F("- wrong slave ID / baud / parity / stop bits"));
  Serial.println(F("- A/B polarity"));
  Serial.println(F("- RS485 wiring / device power"));
  Serial.println(F("- DE/RE timing / transceiver issue"));
}

bool ModbusClient::runCrcSelfTest() {
  Serial.println();
  Serial.println(F("## CRC SELF TEST"));
  Serial.println();

  uint8_t fc03[] = {0x32, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  const size_t fc03Len = modbusAppendCrc(fc03, 6);
  const bool pass03 =
      (fc03Len == 8 && fc03[6] == 0x81 && fc03[7] == 0xC9 &&
       modbusCrcOk(fc03, 8));

  uint8_t fc04[] = {0x32, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  const size_t fc04Len = modbusAppendCrc(fc04, 6);
  const bool pass04 =
      (fc04Len == 8 && fc04[6] == 0x34 && fc04[7] == 0x09 &&
       modbusCrcOk(fc04, 8));

  const uint8_t exc[] = {0x32, 0x84, 0x02, 0x32, 0xCE};
  const bool passExc = modbusCrcOk(exc, 5);

  Serial.print(F("FC03 request: "));
  Serial.println(pass03 ? F("PASS") : F("FAIL"));
  Serial.print(F("  frame: "));
  printHexLine("", fc03, fc03Len);

  Serial.print(F("FC04 request: "));
  Serial.println(pass04 ? F("PASS") : F("FAIL"));
  Serial.print(F("  frame: "));
  printHexLine("", fc04, fc04Len);

  Serial.print(F("FC04 exception frame: "));
  Serial.println(passExc ? F("PASS") : F("FAIL"));
  Serial.print(F("  frame: "));
  printHexLine("", exc, 5);

  const bool all = pass03 && pass04 && passExc;
  Serial.println();
  Serial.print(F("Overall: "));
  Serial.println(all ? F("PASS") : F("FAIL — fix CRC before physical debug"));
  return all;
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

  Serial.println(F("========================================"));
  Serial.println(F("MODBUS RTU DIAGNOSTIC"));
  Serial.println(F("========================================"));
  if (info && info->deviceName) {
    Serial.print(F("Device: "));
    Serial.println(info->deviceName);
  }
  if (info && info->deviceSerial) {
    Serial.print(F("Serial: "));
    Serial.println(info->deviceSerial);
  }
  Serial.println();
  Serial.print(F("REQUEST: slave="));
  Serial.print(slaveId);
  Serial.print(F(" function=0x"));
  if (functionCode < 0x10) {
    Serial.print('0');
  }
  Serial.print(functionCode, HEX);
  Serial.print(F(" address=0x"));
  Serial.print(address, HEX);
  Serial.print(F(" quantity="));
  Serial.println(quantity);
  if (info && info->baud) {
    Serial.print(F("Baud: "));
    Serial.print(info->baud);
    Serial.print(F(" Format: "));
    Serial.print(info->dataBits);
    Serial.print(info->parity == 1 ? 'O' : (info->parity == 2 ? 'E' : 'N'));
    Serial.println(info->stopBits);
  }
  Serial.println();

  uint8_t req[8];
  req[0] = slaveId;
  req[1] = functionCode;
  req[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
  req[3] = static_cast<uint8_t>(address & 0xFF);
  req[4] = static_cast<uint8_t>((quantity >> 8) & 0xFF);
  req[5] = static_cast<uint8_t>(quantity & 0xFF);
  const size_t reqLen = modbusAppendCrc(req, 6);

  return transact(req, reqLen, slaveId, functionCode, quantity, info);
}

ModbusResponse ModbusClient::transact(const uint8_t *request, size_t requestLen,
                                      uint8_t expectedSlave, uint8_t expectedFc,
                                      uint16_t quantity,
                                      const ModbusTxnInfo *info) {
  (void)info;
  ModbusResponse resp;
  const uint8_t expectedExcFc =
      static_cast<uint8_t>(expectedFc | 0x80);

  dlog("IDLE");
  Serial.println(F("[1] Clear RX buffer"));
  uint8_t stale[16];
  const size_t staleN = rs485.flushRxCounted(stale, sizeof(stale));
  Serial.print(F("RX stale bytes: "));
  Serial.println(staleN);
  if (staleN > 0) {
    Serial.println(F("WARNING: Discarding stale RX bytes:"));
    printHexLine("STALE", stale, staleN > sizeof(stale) ? sizeof(stale) : staleN);
  }
  dlog("RX buffer cleared");

  Serial.println(F("[2] DE/RE -> TX"));
  const uint32_t tDeHigh = micros();
  rs485.setTransmitMode();
  dlog("DE/RE = HIGH");

  Serial.println(F("[3] TX"));
  printHexLine("", request, requestLen);
  const uint32_t tTxStart = micros();
  dlog("TX start");
  rs485.serial().write(request, requestLen);
  rs485.serial().flush();
  delayMicroseconds(RS485_TX_SETTLE_US);
  const uint32_t tTxDone = micros();
  Serial.println(F("[4] TX complete"));
  dlog("TX complete");

  Serial.println(F("[5] DE/RE -> RX"));
  rs485.setReceiveMode();
  const uint32_t tDeLow = micros();
  // Drop TX echo only — no extra delay that could eat a fast reply.
  const size_t echoN = rs485.flushRxCounted(nullptr, 0);
  if (_diagnostic && echoN > 0) {
    Serial.print(F("TX-echo bytes dropped: "));
    Serial.println(echoN);
  }
  dlog("DE/RE = LOW");

  if (_diagnostic) {
    Serial.print(F("TX start:    "));
    Serial.print(tTxStart);
    Serial.println(F(" us"));
    Serial.print(F("TX complete: "));
    Serial.print(tTxDone);
    Serial.println(F(" us"));
    Serial.print(F("DE -> RX:    "));
    Serial.print(tDeLow);
    Serial.println(F(" us"));
    Serial.print(F("DE high held "));
    Serial.print(tDeLow - tDeHigh);
    Serial.println(F(" us"));
  }

  Serial.println(F("[6] Waiting for response..."));
  dlog("RX wait");

  const uint32_t frameSilenceMs = 5;
  const uint32_t start = millis();
  size_t got = 0;
  uint8_t buf[64];
  bool sawByte = false;
  uint32_t lastByteMs = 0;
  uint32_t firstByteMs = 0;

  while ((millis() - start) < MODBUS_RESPONSE_TIMEOUT_MS) {
    bool readSomething = false;
    while (rs485.serial().available() && got < sizeof(buf)) {
      buf[got++] = static_cast<uint8_t>(rs485.serial().read());
      if (!sawByte) {
        firstByteMs = millis();
        dlog("RX_RECEIVING");
      }
      lastByteMs = millis();
      sawByte = true;
      readSomething = true;
    }
    if (sawByte && !readSomething &&
        (millis() - lastByteMs) >= frameSilenceMs) {
      break;
    }
    delay(1);
  }

  delay(2);
  while (rs485.serial().available() && got < sizeof(buf)) {
    buf[got++] = static_cast<uint8_t>(rs485.serial().read());
  }

  resp.rawRxLen = got;
  for (size_t i = 0; i < got; i++) {
    resp.rawRx[i] = buf[i];
  }

  const uint32_t elapsed = millis() - start;

  Serial.println(F("[7] RX"));
  if (got == 0) {
    Serial.println(F("(none)"));
    Serial.print(F("RX length: "));
    Serial.println(got);
    printTimeoutDetail(true, true, 0, elapsed);
    resp.result = ModbusResult::Timeout;
    Serial.print(F("RESULT: "));
    Serial.println(resultName(resp.result));
    Serial.println(F("========================================"));
    return resp;
  }

  printHexLine("", buf, got);
  Serial.print(F("RX length: "));
  Serial.println(got);
  if (_diagnostic && sawByte) {
    Serial.print(F("Time to first RX byte: "));
    Serial.print(firstByteMs - start);
    Serial.println(F(" ms after wait start"));
    for (size_t i = 0; i < got; i++) {
      Serial.print(F("RX byte "));
      Serial.print(i);
      Serial.print(F(": "));
      if (buf[i] < 0x10) {
        Serial.print('0');
      }
      Serial.println(buf[i], HEX);
    }
  }
  dlog("RX complete");

  if (got < 5) {
    Serial.println(F("[8] Frame validation"));
    Serial.println(F("MODBUS PARTIAL RESPONSE"));
    Serial.print(F("RX length: "));
    Serial.println(got);
    resp.result = ModbusResult::Partial;
    Serial.print(F("RESULT: "));
    Serial.println(resultName(resp.result));
    Serial.println(F("========================================"));
    return resp;
  }

  Serial.println(F("[8] Frame validation"));
  dlog("VALIDATE");

  // Candidate = first 5 bytes if exception-shaped, else expected normal length.
  // Also scan for CRC-valid matching frame.
  size_t frameOff = 0;
  size_t frameLen = 0;
  enum class Found : uint8_t { None, Ok, Exception, UnexpectedFc };
  Found found = Found::None;

  for (size_t off = 0; off < got; off++) {
    if (buf[off] != expectedSlave) {
      continue;
    }
    const size_t remain = got - off;
    if (remain < 5) {
      break;
    }

    if (buf[off + 1] == expectedExcFc && modbusCrcOk(buf + off, 5)) {
      frameOff = off;
      frameLen = 5;
      found = Found::Exception;
      break;
    }

    if ((buf[off + 1] & 0x80) && modbusCrcOk(buf + off, 5)) {
      // CRC-valid exception for a different FC
      frameOff = off;
      frameLen = 5;
      found = Found::UnexpectedFc;
      break;
    }

    if (buf[off + 1] == expectedFc) {
      const uint8_t byteCount = buf[off + 2];
      const size_t need = static_cast<size_t>(3 + byteCount + 2);
      if (remain >= need && byteCount == (quantity * 2) &&
          modbusCrcOk(buf + off, need)) {
        frameOff = off;
        frameLen = need;
        found = Found::Ok;
        break;
      }
    }
  }

  // Always show CRC compare on the natural 5-byte window starting at slave ID
  // (or offset 0) for the common CRC-error case.
  size_t crcWinOff = 0;
  for (size_t i = 0; i < got; i++) {
    if (buf[i] == expectedSlave) {
      crcWinOff = i;
      break;
    }
  }
  const size_t crcWinLen =
      (got - crcWinOff >= 5) ? 5 : (got - crcWinOff);
  if (crcWinLen >= 4) {
    printCrcCompare(buf + crcWinOff, crcWinLen);
  }

  if (found == Found::None) {
    Serial.print(F("Slave: "));
    Serial.print(buf[crcWinOff]);
    Serial.println(buf[crcWinOff] == expectedSlave ? F(" (matches request)")
                                                   : F(" (mismatch)"));
    if (got - crcWinOff >= 2) {
      Serial.print(F("Function: 0x"));
      Serial.println(buf[crcWinOff + 1], HEX);
      Serial.print(F("Expected exception FC: 0x"));
      Serial.println(expectedExcFc, HEX);
      Serial.print(F("Expected normal FC: 0x"));
      Serial.println(expectedFc, HEX);
      if ((buf[crcWinOff + 1] & 0x80) &&
          buf[crcWinOff + 1] != expectedExcFc) {
        Serial.println(F("UNEXPECTED FUNCTION RESPONSE (and CRC invalid)"));
      }
    }
    Serial.println(F("RESULT: INVALID MODBUS FRAME"));
    resp.result = ModbusResult::CrcError;
    Serial.println(F("========================================"));
    // Do NOT claim ET1641 exception without CRC validity.
    return resp;
  }

  uint8_t frame[64];
  for (size_t i = 0; i < frameLen; i++) {
    frame[i] = buf[frameOff + i];
  }

  if (frameOff > 0) {
    Serial.print(F("Synced frame at offset "));
    Serial.println(frameOff);
  }

  resp.slaveId = frame[0];
  resp.functionCode = frame[1];

  Serial.print(F("Slave: "));
  Serial.print(resp.slaveId);
  Serial.println(F(" OK"));
  Serial.print(F("Function: 0x"));
  Serial.println(resp.functionCode, HEX);

  if (found == Found::UnexpectedFc) {
    Serial.print(F("Expected: 0x"));
    Serial.println(expectedExcFc, HEX);
    Serial.println(F("UNEXPECTED FUNCTION RESPONSE"));
    Serial.println(F("(CRC valid, but exception FC does not match request)"));
    printCrcCompare(frame, frameLen);
    resp.exceptionCode = frame[2];
    resp.result = ModbusResult::UnexpectedFunction;
    Serial.print(F("RESULT: "));
    Serial.println(resultName(resp.result));
    Serial.println(F("========================================"));
    return resp;
  }

  if (found == Found::Exception) {
    Serial.print(F("Expected: 0x"));
    Serial.println(expectedExcFc, HEX);
    resp.exceptionCode = frame[2];
    Serial.print(F("Exception: 0x"));
    if (resp.exceptionCode < 0x10) {
      Serial.print('0');
    }
    Serial.print(resp.exceptionCode, HEX);
    Serial.print(F(" "));
    Serial.println(exceptionName(resp.exceptionCode));
    printCrcCompare(frame, frameLen);
    resp.result = ModbusResult::Exception;
    Serial.print(F("RESULT: "));
    Serial.println(resultName(resp.result));
    Serial.println(F("(CRC-valid exception — slave heard this request.)"));
    Serial.println(F("========================================"));
    return resp;
  }

  // Normal OK response
  resp.byteCount = frame[2];
  Serial.print(F("Byte Count: "));
  Serial.println(resp.byteCount);
  printCrcCompare(frame, frameLen);

  resp.valueCount = static_cast<uint8_t>(resp.byteCount / 2);
  for (uint8_t i = 0; i < resp.valueCount; i++) {
    const uint16_t hi = frame[3 + (i * 2)];
    const uint16_t lo = frame[4 + (i * 2)];
    resp.values[i] = static_cast<uint16_t>((hi << 8) | lo);
    Serial.print(F("Register["));
    Serial.print(i);
    Serial.print(F("] Raw: 0x"));
    Serial.print(resp.values[i], HEX);
    Serial.print(F("  Decimal: "));
    Serial.println(resp.values[i]);
  }

  resp.result = ModbusResult::Ok;
  Serial.print(F("RESULT: "));
  Serial.println(resultName(resp.result));
  Serial.println(F("========================================"));
  return resp;
}
