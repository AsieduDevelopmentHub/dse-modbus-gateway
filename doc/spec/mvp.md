# DSE 7320 Modbus RTU MVP — ESP32-C3 + MAX485

## Objective

Create a minimal PlatformIO firmware for an ESP32-C3 SuperMini that communicates with a Deep Sea Electronics (DSE) 7320 MKII / 7320 EMF generator controller through a MAX485 TTL-to-RS485 module.

The first MVP must **only establish Modbus RTU communication and allow manual register queries**.

The communication layer is a **generic multi-device Modbus RTU master**: DSE 7320 remains the primary target, with additional RS485 slaves (starting with Invendis ET1641) selectable by configuration. Do not turn the project into an ET1641-only tester.

Do not implement MQTT, Wi-Fi cloud communication, database storage, web UI, OTA, or complex monitoring yet.

---

## Hardware

### MCU

* ESP32-C3 SuperMini
* 3.3 V logic

### RS485 transceiver

* MAX485 module

### Target controller

* DSE 7320 MKII
* DSE 7320 EMF

The firmware should eventually support both models, but initially the communication layer should be generic.

---

# Hardware Connections

## ESP32-C3 → MAX485

| MAX485 | ESP32-C3         |
| ------ | ---------------- |
| VCC    | 3.3V             |
| GND    | GND              |
| DI     | GPIO21 / UART TX |
| RO     | GPIO20 / UART RX |
| DE     | GPIO4            |
| RE     | GPIO4            |

Tie `DE` and `RE` together.

GPIO4 controls RS485 direction:

```text
GPIO4 LOW  = Receive
GPIO4 HIGH = Transmit
```

## MAX485 → DSE Controller

```text
MAX485 A → DSE RS485 A / +
MAX485 B → DSE RS485 B / -
```

If communication does not work, allow A/B to be swapped during testing because RS485 A/B labeling varies between hardware manufacturers.

---

# Firmware Requirements

Use:

* PlatformIO
* Arduino framework
* C++
* ESP32-C3
* Hardware UART
* Modbus RTU

A Modbus library may be used, but keep the implementation simple and transparent.

---

# Configurable Communication Parameters

Create constants/configuration variables for:

```cpp
MODBUS_SLAVE_ID
MODBUS_BAUD_RATE
MODBUS_DATA_BITS
MODBUS_STOP_BITS
MODBUS_PARITY
RS485_RX_PIN
RS485_TX_PIN
RS485_DE_RE_PIN
```

Initial values should be easy to change.

Example:

```cpp
#define RS485_RX_PIN 20
#define RS485_TX_PIN 21
#define RS485_DE_RE_PIN 4

#define MODBUS_SLAVE_ID 10
#define MODBUS_BAUD_RATE 9600
```

Do NOT assume these values are definitely correct for every DSE installation. Make them configurable.

---

# Main MVP Function

The firmware should allow the developer to manually specify:

```text
Slave ID
Function Code
Register Address
Number of Registers
```

For example:

```text
Slave ID:       10
Function Code:  03
Register:       0x0405
Quantity:       1
```

The firmware sends the corresponding Modbus RTU request.

---

# Serial Monitor Interface

Create a simple command-line interface through USB serial.

Example:

```text
DSE 7320 Modbus Tester
----------------------

RS485 RX: GPIO20
RS485 TX: GPIO21
DE/RE:    GPIO4

Baud:     9600
Slave ID: 10

Type 'help' for commands.
>
```

Supported commands should include:

```text
help
status
read <address> <quantity>
readhex <address> <quantity>
config
```

Example:

```text
read 1029 1
```

or:

```text
readhex 0405 1
```

The firmware should send the Modbus request and print the raw response.

---

# Raw Modbus Debugging

This is extremely important for the MVP.

Print:

1. TX bytes
2. RX bytes
3. CRC result
4. Slave ID
5. Function code
6. Register values
7. Modbus exception codes
8. Timeout/error information

Example output:

```text
> readhex 0405 1

TX:
0A 03 04 05 00 01 XX XX

Waiting for response...

RX:
0A 03 02 XX XX XX XX

Slave ID: 10
Function: 03
Byte Count: 2
Register Value: XXXX

CRC: OK
```

Do not hide the raw bytes. They are required for troubleshooting the DSE communication.

---

# Register Address Handling

The DSE documentation may represent registers using hexadecimal addresses such as:

```text
0x0405
```

The firmware should support hexadecimal input.

For example:

```text
readhex 0405 1
```

Internally convert it to the correct Modbus register address expected by the library.

Do NOT automatically assume that a DSE documentation address is identical to the address expected by every Modbus library.

Make the address handling explicit.

---

# Initial Test Register

Use the DSE documentation's battery-voltage register as the first communication test if the supplied register documentation confirms:

```text
0x0405
```

Do not hard-code the interpretation of the returned value until the DSE register documentation confirms its scaling and data type.

Initially display:

```text
Raw register: 0xXXXX
Decimal value: XXXX
```

Later we can implement scaling such as:

```text
raw value × scale factor = engineering value
```

---

# Modbus Error Handling

The firmware must clearly report:

### Timeout

```text
ERROR: Modbus timeout.
Check:
- DSE power
- slave ID
- baud rate
- RS485 A/B wiring
- DE/RE direction
```

### CRC error

```text
ERROR: Invalid Modbus CRC.
```

### Modbus exception

Display the exception code.

For example:

```text
Modbus Exception: 0x02
Illegal Data Address
```

### No response

Do not continuously spam requests.

Wait for the next user command.

---

# Important MVP Restrictions

Do NOT implement:

* MQTT
* Wi-Fi
* HTTP
* cloud backend
* database
* dashboard
* automatic discovery of every DSE register
* automatic polling of hundreds of registers
* OTA
* complex configuration storage

The objective is only to prove:

```text
ESP32-C3
   ↓
UART
   ↓
MAX485
   ↓
RS485
   ↓
DSE 7320
   ↓
Modbus RTU response
   ↓
MAX485
   ↓
ESP32-C3
   ↓
Serial Monitor
```

---

# Code Structure

Keep the project small but modular.

Suggested structure:

```text
default platformio structure but with breakdown files in the src/
config/
modbus/
rs485/
....
```

If a Modbus library is used, isolate it behind `modbus_client`.

The application should not depend directly on library-specific APIs wherever possible.

---

# Future Expansion

The MVP will eventually become a DSE generator monitoring gateway.

Future architecture:

```text
                ┌──────────────────┐
                │   DSE 7320 MKII   │
                └────────┬─────────┘
                         │
                       RS485
                         │
                    ┌────▼─────┐
                    │  MAX485  │
                    └────┬─────┘
                         │ UART
                    ┌────▼─────┐
                    │ ESP32-C3 │
                    └────┬─────┘
                         │
                  Wi-Fi / Ethernet
                         │
                    Cloud Backend
                         │
                    Monitoring UI
```

Eventually create a proper DSE register abstraction:

```cpp
DSEParameter batteryVoltage;
DSEParameter engineRPM;
DSEParameter coolantTemperature;
DSEParameter oilPressure;
DSEParameter fuelLevel;
DSEParameter generatorVoltage;
DSEParameter generatorFrequency;
DSEParameter engineHours;
DSEParameter alarms;
```

But this is **not part of the first MVP**.

---

# Important Development Rule

Do not guess DSE register addresses, scaling factors, data types, or byte ordering.

The firmware should be designed as a **generic Modbus RTU register tester first**.

Once communication is confirmed, we will populate a proper DSE 7320 register map from the official DSE documentation and build the monitoring layer around it.

---

# Expected MVP Success Criteria

The MVP is successful when the ESP32-C3 can:

1. Initialize the MAX485.
2. Control RS485 transmit/receive direction.
3. Send a valid Modbus RTU request.
4. Receive a response from the DSE controller.
5. Validate CRC.
6. Display raw response bytes.
7. Decode the returned register.
8. Allow different registers to be queried without recompiling the firmware.
9. Clearly report timeout, CRC, and Modbus exception errors.

Nothing more is required at this stage.
