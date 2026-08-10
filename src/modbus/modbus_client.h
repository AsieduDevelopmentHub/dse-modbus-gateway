#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class ModbusResult : uint8_t {
  Ok = 0,
  Timeout,
  CrcError,
  Exception,
  FrameError,
  InvalidArgs,
  UnexpectedFunction,
  Partial,
};

struct ModbusResponse {
  ModbusResult result = ModbusResult::Timeout;
  uint8_t slaveId = 0;
  uint8_t functionCode = 0;
  uint8_t exceptionCode = 0;
  uint8_t byteCount = 0;
  uint16_t values[32] = {};
  uint8_t valueCount = 0;
  uint8_t rawRx[64] = {};
  size_t rawRxLen = 0;
};

struct ModbusTxnInfo {
  const char *deviceName = nullptr;
  const char *deviceSerial = nullptr;
  uint32_t baud = 0;
  uint8_t dataBits = 8;
  uint8_t parity = 0;
  uint8_t stopBits = 1;
  uint16_t address = 0;
  uint16_t quantity = 0;
};

class ModbusClient {
public:
  void begin();

  void setDiagnostic(bool enabled);
  bool diagnostic() const { return _diagnostic; }

  // Offline CRC vectors — no RS485 required.
  bool runCrcSelfTest();

  ModbusResponse readHoldingRegisters(uint8_t slaveId, uint16_t address,
                                      uint16_t quantity,
                                      const ModbusTxnInfo *info = nullptr);

  ModbusResponse readInputRegisters(uint8_t slaveId, uint16_t address,
                                    uint16_t quantity,
                                    const ModbusTxnInfo *info = nullptr);

  ModbusResponse readRegisters(uint8_t slaveId, uint8_t functionCode,
                               uint16_t address, uint16_t quantity,
                               const ModbusTxnInfo *info = nullptr);

private:
  ModbusResponse transact(const uint8_t *request, size_t requestLen,
                          uint8_t expectedSlave, uint8_t expectedFc,
                          uint16_t quantity, const ModbusTxnInfo *info);

  void dlog(const char *msg);
  void printHexLine(const char *label, const uint8_t *data, size_t len);
  void printCrcCompare(const uint8_t *frame, size_t length);
  void printTimeoutDetail(bool txDone, bool deToRx, size_t rxLen,
                          uint32_t elapsedMs);
  const char *exceptionName(uint8_t code);
  const char *resultName(ModbusResult r);

  bool _diagnostic = false;
};

extern ModbusClient modbus;
