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
// Match doc/guide/getting_started.md. GPIO6/7 was tried and timed out on
// this bench because the transceiver stays on 20/21/4.
#ifndef RS485_RX_PIN
#define RS485_RX_PIN 20
#endif
#ifndef RS485_TX_PIN
#define RS485_TX_PIN 21
#endif
#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN 4
#endif
// Prefer a 3.3V transceiver (THVD1450DR) on ESP32-C3. MAX485 RO at 5V VCC
// still swings ~5V into a 3.3V-only GPIO and can corrupt trailing CRC bytes.
#define MAX485_VCC_NOTE "THVD1450: 3.3V VCC OK; MAX485: 5V VBUS preferred"
#define MAX485_RO_NOTE "THVD1450 RO is 3.3V-safe; MAX485 RO is 5V-domain RISK"
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

// Post-TX turnaround before DE→RX. Keep this SHORT: Modbus slaves may reply
// after ~3.5 character times (~3.5 ms @ 9600). Holding DE high that long
// causes bus contention and CRC garbage. THVD1450 disables in tens of ns;
// a few character-bit times of margin is enough after uart_wait_tx_done.
#ifndef RS485_TX_SETTLE_US
#define RS485_TX_SETTLE_US 200
#endif

// Brief pause after DE assert before first TX byte (driver enable).
#ifndef RS485_TX_PRE_DELAY_US
#define RS485_TX_PRE_DELAY_US 50
#endif

// USB Serial (CLI) — baud ignored on native USB CDC (ESP32-C3)
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif

// ---------------------------------------------------------------------------
// Wi-Fi / local web UI (ESP32-C3 only)
// Soft-AP always starts. Optional STA: set WIFI_STA_SSID (non-empty) to join.
// ---------------------------------------------------------------------------
#ifndef WIFI_AP_SSID_PREFIX
#define WIFI_AP_SSID_PREFIX "DSE-GW"
#endif
#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD "dsegateway"
#endif
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID ""
#endif
#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD ""
#endif
#ifndef HTTP_SERVER_PORT
#define HTTP_SERVER_PORT 80
#endif
