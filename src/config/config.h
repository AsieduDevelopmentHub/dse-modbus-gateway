#pragma once

// ---------------------------------------------------------------------------
// Hardware pins — ESP32-C3 SuperMini ↔ MAX485
// ---------------------------------------------------------------------------
#ifndef RS485_RX_PIN
#define RS485_RX_PIN 20
#endif

#ifndef RS485_TX_PIN
#define RS485_TX_PIN 21
#endif

#ifndef RS485_DE_RE_PIN
#define RS485_DE_RE_PIN 4
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

// USB Serial (CLI)
#ifndef USB_SERIAL_BAUD
#define USB_SERIAL_BAUD 115200
#endif

// Hardware UART for RS485 (UART1 on ESP32-C3; USB CDC uses UART0)
#ifndef RS485_UART_NUM
#define RS485_UART_NUM 1
#endif
