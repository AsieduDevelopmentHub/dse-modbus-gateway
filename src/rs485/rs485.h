#pragma once

#include <Arduino.h>
#include "config/config.h"

// MAX485 + UART1 (HardwareSerial).
// DE/RE GPIO4: HIGH=TX, LOW=RX (tied together on module).
//
// HARDWARE RISK (documented, not changed here):
// - MAX485 module powered from ESP32-C3 3.3V
// - GPIO20/21 are also ESP32-C3 default UART0 pins

class Rs485Bus {
public:
  void begin();
  void applyConfig(uint32_t baud, uint8_t dataBits, uint8_t parity,
                   uint8_t stopBits);

  void setTransmitMode();
  void setReceiveMode();
  int deReLevel() const;  // digitalRead of DE/RE pin

  HardwareSerial &serial();
  void flushRx();
  size_t flushRxCounted(uint8_t *outBuf = nullptr, size_t outMax = 0);

  // TX with DE high, hardware TX-done wait, then DE low.
  // Returns bytes accepted by write(); sets outTxCompleteUs.
  size_t transmit(const uint8_t *data, size_t len, uint32_t *outTxStartUs,
                  uint32_t *outTxCompleteUs, uint32_t *outDeLowUs);

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
