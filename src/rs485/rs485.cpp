#include "rs485/rs485.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "driver/uart.h"
#endif

Rs485Bus rs485;

#if defined(RS485_USE_SOFT_SERIAL)
Rs485Bus::Rs485Bus() : _soft(RS485_RX_PIN, RS485_TX_PIN) {}
#elif defined(ARDUINO_ARCH_ESP32)
Rs485Bus::Rs485Bus() : _hw(RS485_UART_NUM) {}
#else
Rs485Bus::Rs485Bus() : _hw(Serial1) {}
#endif

#if defined(ARDUINO_ARCH_ESP32)
SerialConfig Rs485Bus::makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                        uint8_t stopBits) {
#else
uint8_t Rs485Bus::makeSerialConfig(uint8_t dataBits, uint8_t parity,
                                   uint8_t stopBits) {
#endif
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

Stream &Rs485Bus::stream() {
#if defined(RS485_USE_SOFT_SERIAL)
  return _soft;
#else
  return _hw;
#endif
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

#if defined(RS485_USE_SOFT_SERIAL)
  _soft.end();
  delay(10);
  // SoftwareSerial is 8N1 only — parity/stop overrides are ignored.
  (void)parity;
  (void)stopBits;
  (void)dataBits;
  _soft.begin(baud);
#else
  _hw.end();
  delay(10);
#if defined(ARDUINO_ARCH_ESP32)
  // One-byte FIFO so each RX byte is visible as it arrives. Gaps then
  // match the wire (~1.1 ms at 9600). A 120-byte FIFO hides that and made
  // trailing noise look like CRC bytes.
  _hw.begin(baud, makeSerialConfig(dataBits, parity, stopBits), RS485_RX_PIN,
            RS485_TX_PIN, false, 20000UL, 1);
  _hw.setRxFIFOFull(1);
  _hw.setRxTimeout(2);
  pinMode(RS485_RX_PIN, INPUT_PULLUP);
  uart_flush_input(static_cast<uart_port_t>(RS485_UART_NUM));
#else
  _hw.begin(baud, makeSerialConfig(dataBits, parity, stopBits));
#endif
#endif
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

void Rs485Bus::flushRx() {
  (void)flushRxCounted(nullptr, 0);
}

size_t Rs485Bus::flushRxCounted(uint8_t *outBuf, size_t outMax) {
  size_t n = 0;
  Stream &s = stream();
  while (s.available()) {
    const uint8_t b = static_cast<uint8_t>(s.read());
    if (outBuf && n < outMax) {
      outBuf[n] = b;
    }
    n++;
  }
  return n;
}

void Rs485Bus::waitTxComplete() {
#if defined(RS485_USE_SOFT_SERIAL)
  // SoftwareSerial::flush() waits until TX is idle.
  _soft.flush();
#elif defined(ARDUINO_ARCH_ESP32)
  _hw.flush();
  uart_wait_tx_done(static_cast<uart_port_t>(RS485_UART_NUM),
                    pdMS_TO_TICKS(100));
#else
  _hw.flush();
#endif
}

size_t Rs485Bus::transmit(const uint8_t *data, size_t len,
                          uint32_t *outTxStartUs, uint32_t *outTxCompleteUs,
                          uint32_t *outDeLowUs) {
#if defined(ARDUINO_ARCH_ESP32)
  // Drop a stuck RX FIFO. An overflow during USB printing otherwise
  // swallows the next slave reply and a write looks like a timeout.
  uart_flush_input(static_cast<uart_port_t>(RS485_UART_NUM));
#endif
  setTransmitMode();
  if (_txPreDelayUs > 0) {
    delayMicroseconds(_txPreDelayUs);
  }
  const uint32_t t0 = micros();
  if (outTxStartUs) {
    *outTxStartUs = t0;
  }

  const size_t written = stream().write(data, len);
  waitTxComplete();
  if (_txSettleUs > 0) {
    delayMicroseconds(_txSettleUs);
  }

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
