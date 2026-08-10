#include "modbus/modbus_crc.h"

uint16_t modbusCrc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

size_t modbusAppendCrc(uint8_t *frame, size_t length) {
  const uint16_t crc = modbusCrc16(frame, length);
  frame[length++] = static_cast<uint8_t>(crc & 0xFF);        // CRC Lo
  frame[length++] = static_cast<uint8_t>((crc >> 8) & 0xFF);  // CRC Hi
  return length;
}

bool modbusCrcOk(const uint8_t *frame, size_t length) {
  if (length < 4) {
    return false;
  }
  const uint16_t received =
      static_cast<uint16_t>(frame[length - 2]) |
      (static_cast<uint16_t>(frame[length - 1]) << 8);
  const uint16_t calculated = modbusCrc16(frame, length - 2);
  return received == calculated;
}
