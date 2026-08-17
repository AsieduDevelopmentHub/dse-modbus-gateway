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
  uint32_t txnId = 0;
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
  bool quiet = false;  // suppress verbose TX/RX dump (iterate/batch)
};

struct RawRs485Capture {
  uint8_t tx[16] = {};
  size_t txLen = 0;
  size_t txWritten = 0;
  uint8_t rx[64] = {};
  size_t rxLen = 0;
  uint32_t txnId = 0;
  uint32_t tTxStartUs = 0;
  uint32_t tTxCompleteUs = 0;
  uint32_t tDeLowUs = 0;
  uint32_t tFirstRxMs = 0;
  uint32_t elapsedMs = 0;
  int deBefore = 0;
  int deAfter = 0;
};

class ModbusClient {
public:
  void begin();

  void setDiagnostic(bool enabled);
  bool diagnostic() const { return _diagnostic; }
  uint32_t nextTxnId() { return ++_txnId; }

  bool runCrcSelfTest();

  // Build FC03/04 request, TX, capture raw RX only (no Modbus parse).
  RawRs485Capture rawCapture(uint8_t slaveId, uint8_t functionCode,
                             uint16_t address, uint16_t quantity);

  ModbusResponse readHoldingRegisters(uint8_t slaveId, uint16_t address,
                                      uint16_t quantity,
                                      const ModbusTxnInfo *info = nullptr);

  ModbusResponse readInputRegisters(uint8_t slaveId, uint16_t address,
                                    uint16_t quantity,
                                    const ModbusTxnInfo *info = nullptr);

  ModbusResponse readRegisters(uint8_t slaveId, uint8_t functionCode,
                               uint16_t address, uint16_t quantity,
                               const ModbusTxnInfo *info = nullptr);

  // FC16 write multiple holding registers (quantity 1..16).
  ModbusResponse writeMultipleRegisters(uint8_t slaveId, uint16_t address,
                                        const uint16_t *values,
                                        uint16_t quantity,
                                        const ModbusTxnInfo *info = nullptr);

  // Validate CRC on an already-captured buffer (for rs485raw follow-up).
  void printCrcAudit(const uint8_t *frame, size_t length);

private:
  ModbusResponse transact(const uint8_t *request, size_t requestLen,
                          uint8_t expectedSlave, uint8_t expectedFc,
                          uint16_t quantity, const ModbusTxnInfo *info,
                          bool writeResponse);

  void dlog(const char *msg);
  void printHexLine(const char *label, const uint8_t *data, size_t len);
  void printCrcCompare(const uint8_t *frame, size_t length);
  void printTimeoutDetail(bool txDone, bool deToRx, size_t rxLen,
                          uint32_t elapsedMs);
  size_t collectRx(uint8_t *buf, size_t bufMax, uint32_t timeoutMs,
                   uint32_t *outFirstByteMs);
  const char *exceptionName(uint8_t code);
  const char *resultName(ModbusResult r);

  bool _diagnostic = false;
  uint32_t _txnId = 0;
};

extern ModbusClient modbus;
