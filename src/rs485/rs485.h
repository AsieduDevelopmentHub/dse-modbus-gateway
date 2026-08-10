#pragma once

#include <Arduino.h>
#include "config/config.h"

// MAX485 direction control + HardwareSerial (UART1).
// GPIO4 LOW  = Receive (RE/DE tied)
// GPIO4 HIGH = Transmit
//
// Physical note (Phase 2A): MAX485 module is powered from ESP32-C3 3.3V.
// Do not claim automatic electrical safety/compatibility beyond that setup.

class Rs485Bus {
public:
  void begin();
  void applyConfig(uint32_t baud, uint8_t dataBits, uint8_t parity,
                   uint8_t stopBits);

  void setTransmitMode();
  void setReceiveMode();

  HardwareSerial &serial();
  void flushRx();
  // Drain RX FIFO; returns count discarded. Optionally fills outBuf.
  size_t flushRxCounted(uint8_t *outBuf = nullptr, size_t outMax = 0);

  uint32_t baud() const { return _baud; }
  uint8_t dataBits() const { return _dataBits; }
  uint8_t parity() const { return _parity; }
  uint8_t stopBits() const { return _stopBits; }

private:
  static SerialConfig makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                       uint8_t stopBits);

  HardwareSerial _serial{RS485_UART_NUM};
  uint32_t _baud = MODBUS_BAUD_RATE;
  uint8_t _dataBits = MODBUS_DATA_BITS;
  uint8_t _parity = MODBUS_PARITY;
  uint8_t _stopBits = MODBUS_STOP_BITS;
};

extern Rs485Bus rs485;
