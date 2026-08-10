#pragma once

#include <Arduino.h>
#include <stdint.h>

// Modbus RTU CRC-16 (poly 0xA001, init 0xFFFF)
uint16_t modbusCrc16(const uint8_t *data, size_t length);

// Append CRC (low byte first) to buffer; returns new length (= length + 2).
size_t modbusAppendCrc(uint8_t *frame, size_t length);

// Validate CRC of a complete frame (including CRC bytes at end).
bool modbusCrcOk(const uint8_t *frame, size_t length);
