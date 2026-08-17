#include "rs485/rs485.h"

#include "driver/uart.h"

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

int Rs485Bus::deReLevel() const {
  return digitalRead(RS485_DE_RE_PIN);
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

size_t Rs485Bus::transmit(const uint8_t *data, size_t len,
                          uint32_t *outTxStartUs, uint32_t *outTxCompleteUs,
                          uint32_t *outDeLowUs) {
  setTransmitMode();
  const uint32_t t0 = micros();
  if (outTxStartUs) {
    *outTxStartUs = t0;
  }

  const size_t written = _serial.write(data, len);
  _serial.flush();  // Arduino: wait until TX FIFO drained

  // ESP-IDF: wait until shift register finished last stop bit.
  uart_wait_tx_done(static_cast<uart_port_t>(RS485_UART_NUM),
                    pdMS_TO_TICKS(100));

  delayMicroseconds(RS485_TX_SETTLE_US);

  const uint32_t t1 = micros();
  if (outTxCompleteUs) {
    *outTxCompleteUs = t1;
  }

  setReceiveMode();
  const uint32_t t2 = micros();
  if (outDeLowUs) {
    *outDeLowUs = t2;
  }

  return written;
}
