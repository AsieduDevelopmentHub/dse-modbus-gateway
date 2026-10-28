#pragma once

#include <Arduino.h>
#include "config/config.h"

#if defined(RS485_USE_SOFT_SERIAL)
#include <SoftwareSerial.h>
#endif

// Half-duplex RS485 + UART direction control (THVD1450DR / MAX485-class).
// DE/RE: HIGH=TX, LOW=RX (tie DE and /RE together on the transceiver).
//
// ESP32-C3: prefer THVD1450 (3.3V RO). MAX485 with VCC=5V still drives ~5V
// into a 3.3V GPIO and can corrupt trailing CRC bytes.

class Rs485Bus {
public:
  Rs485Bus();

  void begin();
  void applyConfig(uint32_t baud, uint8_t dataBits, uint8_t parity,
                   uint8_t stopBits);

  void setTransmitMode();
  void setReceiveMode();
  int deReLevel() const;

  Stream &stream();
  void flushRx();
  size_t flushRxCounted(uint8_t *outBuf = nullptr, size_t outMax = 0);

  // TX with DE high, wait TX complete, settle, then DE low.
  size_t transmit(const uint8_t *data, size_t len, uint32_t *outTxStartUs,
                  uint32_t *outTxCompleteUs, uint32_t *outDeLowUs);

  uint32_t baud() const { return _baud; }
  uint8_t dataBits() const { return _dataBits; }
  uint8_t parity() const { return _parity; }
  uint8_t stopBits() const { return _stopBits; }

  uint32_t txSettleUs() const { return _txSettleUs; }
  uint32_t txPreDelayUs() const { return _txPreDelayUs; }
  void setTxSettleUs(uint32_t us) { _txSettleUs = us; }
  void setTxPreDelayUs(uint32_t us) { _txPreDelayUs = us; }

private:
#if defined(ARDUINO_ARCH_ESP32)
  static SerialConfig makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                       uint8_t stopBits);
#else
  static uint8_t makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                  uint8_t stopBits);
#endif

  void waitTxComplete();

#if defined(RS485_USE_SOFT_SERIAL)
  SoftwareSerial _soft;
#elif defined(ARDUINO_ARCH_ESP32)
  HardwareSerial _hw;
#else
  // Mega / boards with Serial1
  HardwareSerial &_hw;
#endif

  uint32_t _baud = MODBUS_BAUD_RATE;
  uint8_t _dataBits = MODBUS_DATA_BITS;
  uint8_t _parity = MODBUS_PARITY;
  uint8_t _stopBits = MODBUS_STOP_BITS;
  uint32_t _txSettleUs = RS485_TX_SETTLE_US;
  uint32_t _txPreDelayUs = RS485_TX_PRE_DELAY_US;
};

extern Rs485Bus rs485;
