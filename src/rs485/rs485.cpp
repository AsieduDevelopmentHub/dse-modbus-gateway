#include "rs485/rs485.h"

Rs485Bus rs485;

SerialConfig Rs485Bus::makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                        uint8_t stopBits) {
  if (dataBits == 8 && stopBits == 1) {
    if (parity == 1) {
      return SERIAL_8O1;
    }
    if (parity == 2) {
      return SERIAL_8E1;
    }
    return SERIAL_8N1;
  }
  if (dataBits == 8 && stopBits == 2) {
    if (parity == 1) {
      return SERIAL_8O2;
    }
    if (parity == 2) {
      return SERIAL_8E2;
    }
    return SERIAL_8N2;
  }
  return SERIAL_8N1;
}

void Rs485Bus::begin() {
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  setReceiveMode();
  applyConfig(MODBUS_BAUD_RATE, MODBUS_DATA_BITS, MODBUS_PARITY,
              MODBUS_STOP_BITS);
}

void Rs485Bus::applyConfig(uint32_t baud, uint8_t dataBits, uint8_t parity,
                           uint8_t stopBits) {
  _baud = baud;
  _dataBits = dataBits;
  _parity = parity;
  _stopBits = stopBits;

  _serial.end();
  delay(10);
  _serial.begin(baud, makeSerialConfig(dataBits, parity, stopBits),
                RS485_RX_PIN, RS485_TX_PIN);
  flushRx();
}

void Rs485Bus::setTransmitMode() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);
}

void Rs485Bus::setReceiveMode() {
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

HardwareSerial &Rs485Bus::serial() {
  return _serial;
}

void Rs485Bus::flushRx() {
  (void)flushRxCounted(nullptr, 0);
}

size_t Rs485Bus::flushRxCounted(uint8_t *outBuf, size_t outMax) {
  size_t n = 0;
  while (_serial.available()) {
    const uint8_t b = static_cast<uint8_t>(_serial.read());
    if (outBuf && n < outMax) {
      outBuf[n] = b;
    }
    n++;
  }
  return n;
}
