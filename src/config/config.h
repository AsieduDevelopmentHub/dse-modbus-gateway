#pragma once

// ---------------------------------------------------------------------------
// Board profiles (set via platformio.ini build_flags)
//   BOARD_PROFILE_ESP32C3  — ESP32-C3 SuperMini (default if none set)
//   BOARD_PROFILE_MEGA     — Arduino Mega 2560 (5V, HardwareSerial1)
//   BOARD_PROFILE_UNO      — Arduino Uno (5V, SoftwareSerial)
//   BOARD_PROFILE_NANO     — Arduino Nano (5V, SoftwareSerial)
// ---------------------------------------------------------------------------

#if defined(BOARD_PROFILE_MEGA)
#define BOARD_NAME "Arduino Mega 2560"
#define RS485_UART_LABEL "Serial1 (HW)"
#ifndef RS485_RX_PIN
#define RS485_RX_PIN 19  // Mega Serial1 RX
#endif
#ifndef RS485_TX_PIN
#define RS485_TX_PIN 18  // Mega Serial1 TX
#endif
#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN 2
#endif
#define MAX485_VCC_NOTE "5V (Arduino 5V pin — required)"
#define MAX485_RO_NOTE "5V logic OK (Arduino is 5V)"

#elif defined(BOARD_PROFILE_UNO) || defined(BOARD_PROFILE_NANO)
#if defined(BOARD_PROFILE_NANO)
#define BOARD_NAME "Arduino Nano"
#else
#define BOARD_NAME "Arduino Uno"
#endif
#define RS485_USE_SOFT_SERIAL 1
#define RS485_UART_LABEL "SoftwareSerial"
#ifndef RS485_RX_PIN
#define RS485_RX_PIN 10
#endif
#ifndef RS485_TX_PIN
#define RS485_TX_PIN 11
#endif
#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN 2
#endif
#define MAX485_VCC_NOTE "5V (Arduino 5V pin — required)"
#define MAX485_RO_NOTE "5V logic OK (Arduino is 5V)"

#else
// Default: ESP32-C3 SuperMini
#ifndef BOARD_PROFILE_ESP32C3
#define BOARD_PROFILE_ESP32C3 1
#endif
#define BOARD_NAME "ESP32-C3 SuperMini"
#define RS485_UART_LABEL "UART1"
#ifndef RS485_RX_PIN
#define RS485_RX_PIN 20
#endif
#ifndef RS485_TX_PIN
#define RS485_TX_PIN 21
#endif
#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN 4
#endif
// CRITICAL: VCC=5V does NOT make RO safe for ESP32-C3. RO still swings ~5V
// into a 3.3V-only GPIO and can corrupt the last bytes (CRC) of a frame.
#define MAX485_VCC_NOTE "5V VBUS preferred for MAX485 VCC (not 3.3V)"
#define MAX485_RO_NOTE "RISK: RO is 5V-domain — ESP32-C3 GPIO is 3.3V only"
#endif

// Hardware UART index (ESP32 only; Mega uses Serial1 object)
#ifndef RS485_UART_NUM
#define RS485_UART_NUM 1
#endif

// ---------------------------------------------------------------------------
// Modbus RTU defaults — DSE site default (overridable via CLI set / device)
// ---------------------------------------------------------------------------
#ifndef MODBUS_SLAVE_ID
#define MODBUS_SLAVE_ID 1
#endif

#ifndef MODBUS_BAUD_RATE
#define MODBUS_BAUD_RATE 9600
#endif

#ifndef MODBUS_DATA_BITS
#define MODBUS_DATA_BITS 8
#endif

#ifndef MODBUS_STOP_BITS
#define MODBUS_STOP_BITS 1
#endif

// 0 = None, 1 = Odd, 2 = Even
#ifndef MODBUS_PARITY
#define MODBUS_PARITY 0
#endif

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
#ifndef MODBUS_RESPONSE_TIMEOUT_MS
#define MODBUS_RESPONSE_TIMEOUT_MS 1000
#endif

// Post-TX turnaround before switching to RX (MAX485 settle).
// At 9600 baud, ~1 char ≈ 1 ms; allow a few character times.
#ifndef RS485_TX_SETTLE_US
#define RS485_TX_SETTLE_US 3500
#endif

// USB Serial (CLI) — baud ignored on native USB CDC (ESP32-C3)
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif
