# PHASE 2 IMPLEMENTATION TASK
# DSE 7320 + Invendis ET1641 Multi-Device Modbus RTU Test

## Project

Repository:
dse7320_rtu

Current phase:
MVP Phase 1 — Firmware + USB CLI bring-up COMPLETE

We are now moving to:

PHASE 2 — Physical RS485 / Modbus RTU validation using an Invendis ET1641 DC Energy Meter.

IMPORTANT:
The Invendis ET1641 is NOT replacing the DSE 7320.

The DSE 7320 MKII / 7320 EMF remains a primary target of this project.

The ET1641 is being added as the FIRST additional RS485/Modbus device so that the ESP32-C3 gateway can be tested against a real Modbus device before the DSE controller is physically available.

The architecture must therefore evolve from a "DSE tester" into a GENERIC MULTI-DEVICE MODBUS RTU TESTER/GATEWAY.

--------------------------------------------------
CURRENT PHASE 1 STATUS
--------------------------------------------------

Phase 1 is already complete.

The current firmware:

- Builds with PlatformIO
- Runs on ESP32-C3 SuperMini
- Uses Arduino framework
- Uses native USB CDC
- Has a working USB CLI
- Uses custom Modbus RTU implementation
- Supports Modbus FC 03 and FC 04
- Generates Modbus CRC
- Controls RS485 DE/RE
- Prints raw TX/RX frames
- Validates CRC
- Handles timeout/error/exception cases
- Allows manual register queries

Current project structure:

platformio.ini
src/
  main.cpp
  config/config.h
  rs485/rs485.h
  rs485/rs485.cpp
  modbus/modbus_crc.h
  modbus/modbus_crc.cpp
  modbus/modbus_client.h
  modbus/modbus_client.cpp
  cli/cli.h
  cli/cli.cpp

doc/
  spec/mvp.md
  status/phase_status.md
  guide/getting_started.md

DO NOT unnecessarily rewrite or replace the existing working Modbus implementation.

Inspect the existing code first and build Phase 2 on top of it.

--------------------------------------------------
CURRENT ESP32-C3 HARDWARE
--------------------------------------------------

Board:

ESP32-C3 SuperMini

RS485 UART:

UART1

Pins:

RX = GPIO20
TX = GPIO21
DE/RE = GPIO4

MAX485:

The physical MAX485 module being used is a common 5V MAX485 TTL-to-RS485 module.

MAX485 VCC is connected to the ESP32-C3 5V/VBUS supply.

MAX485 GND is connected to ESP32 GND.

MAX485:

DI -> ESP32 GPIO21 / UART TX
RO -> ESP32 GPIO20 / UART RX
DE -> GPIO4
RE -> GPIO4

DE and RE are tied together.

Current direction logic:

GPIO4 LOW  = receive
GPIO4 HIGH = transmit

IMPORTANT HARDWARE NOTE:

Do not change the existing power arrangement or introduce a different RS485 transceiver in this phase.

Do not redesign the hardware.

However, document clearly that the MAX485 module is a 5V device and that its RO logic-level compatibility with the ESP32-C3 must be treated as a hardware consideration.

Do not claim that the 5V MAX485 logic interface is automatically safe for ESP32-C3 GPIO.

The objective of this phase is software + communication validation with the existing hardware.

--------------------------------------------------
CURRENT DEFAULT MODBUS SETTINGS
--------------------------------------------------

Existing DSE-oriented defaults must remain available.

Current defaults:

Baud:
9600

Format:
8N1

Default DSE slave ID:
10

Default function code:
03

Response timeout:
1000 ms

IMPORTANT:

Do NOT replace the DSE defaults with ET1641 settings globally.

The ET1641 must be represented as a separate device configuration.

--------------------------------------------------
ET1641 DEVICE TO ADD
--------------------------------------------------

Physical device:

Manufacturer:
Invendis

Model:
ET1641

Device type:
DC Energy Meter / DCEM

Serial Number:
725093

Modbus:

Protocol:
Modbus RTU

Slave ID:
50

Baud:
9600

Parity:
None

Stop bits:
1

Data format:
8N1

The ET1641 is currently the physical device available for Phase 2 testing.

--------------------------------------------------
ARCHITECTURAL REQUIREMENT
--------------------------------------------------

The firmware should no longer assume that the ESP32 is talking only to one DSE device.

The communication layer must support multiple Modbus devices.

Conceptually:

ESP32-C3
   |
   | UART
   |
MAX485
   |
RS485 BUS
   |
   +---- DSE 7320 MKII / EMF
   |
   +---- Invendis ET1641
   |
   +---- Future Modbus device
   |
   +---- Future Modbus device

Example configuration:

DSE 7320:
    device type = DSE7320
    slave ID = 10
    baud = 9600
    parity = none
    stop bits = 1

Invendis ET1641:
    device type = ET1641
    slave ID = 50
    baud = 9600
    parity = none
    stop bits = 1

Future devices must be possible without rewriting the Modbus transport layer.

--------------------------------------------------
DO NOT IMPLEMENT DEVICE REGISTER MAPS YET
--------------------------------------------------

This is very important.

DO NOT guess:

- ET1641 register addresses
- DSE register addresses
- scaling factors
- engineering units
- byte ordering
- IEEE-754 register ordering
- battery voltage register
- current register
- power register
- energy register

The purpose of Phase 2 is to prove physical Modbus communication first.

We will add the official register maps after receiving a valid response.

The previous DSE test address:

0x0405

must NOT automatically be used for ET1641.

Treat addresses as device-specific.

--------------------------------------------------
PHASE 2 OBJECTIVE
--------------------------------------------------

The primary objective is:

PROVE THAT:

ESP32-C3
    ->
UART
    ->
5V MAX485
    ->
RS485
    ->
Invendis ET1641
    ->
RS485
    ->
MAX485
    ->
ESP32-C3

can successfully exchange valid Modbus RTU frames.

A successful Phase 2 result is:

1. ESP32 sends a valid Modbus request.
2. ET1641 receives it.
3. ET1641 responds.
4. ESP32 receives the response.
5. CRC validates.
6. Raw RX bytes are displayed.
7. Slave ID and function code are displayed.
8. Communication errors are clearly reported.

DO NOT require meaningful engineering values yet.

--------------------------------------------------
CLI ENHANCEMENT
--------------------------------------------------

Preserve all existing CLI functionality.

Add device-oriented commands.

The CLI should support something conceptually like:

devices

device list

device select <name>

device show

read <address> <quantity>

readhex <address> <quantity>

config

status

help

For example:

devices

Expected concept:

Available devices:

[0] DSE 7320 MKII
    Slave ID: 10
    Baud: 9600
    Format: 8N1

[1] Invendis ET1641
    Slave ID: 50
    Baud: 9600
    Format: 8N1

Then:

device select et1641

should select the ET1641 configuration for the next queries.

Then:

device show

should display:

Device: Invendis ET1641
Model: ET1641
Serial: 725093
Slave ID: 50
Baud: 9600
Parity: None
Stop bits: 1

Then the existing read commands can be used.

Example:

readhex <register> 1

IMPORTANT:

Do not require a register address to be known merely to test the physical link.

If necessary, add a low-level raw Modbus command for testing.

--------------------------------------------------
OPTIONAL LOW-LEVEL TEST COMMAND
--------------------------------------------------

If practical, add:

rawread <slave> <function> <address> <quantity>

Example:

rawread 50 03 0000 1

This should generate the Modbus RTU request directly.

The purpose is diagnostic testing.

Do not claim register 0000 is a valid ET1641 register.

If the device responds with a Modbus exception, that is still useful because it proves the slave received the request and responded.

For example:

RX:
32 83 02 CRC CRC

should be interpreted as:

Slave: 50
Exception response
Function: 0x03
Exception: 0x02

This is still evidence of physical Modbus communication.

--------------------------------------------------
DEVICE CONFIGURATION DESIGN
--------------------------------------------------

Introduce a lightweight device configuration abstraction.

For example:

struct ModbusDeviceConfig {
    const char* name;
    const char* model;
    const char* serial;
    uint8_t slaveId;
    uint32_t baudRate;
    uint8_t parity;
    uint8_t stopBits;
};

Do not over-engineer this.

No database.

No JSON configuration system.

No cloud configuration.

No persistence yet.

Keep configuration compile-time/static for now.

--------------------------------------------------
UART CONFIGURATION
--------------------------------------------------

The transport should support:

9600 8N1

for both current devices.

However, design the configuration so that baud/parity/stop bits are properties of the selected device.

Do not hard-code ET1641 values into the global Modbus transport.

--------------------------------------------------
RS485 TRANSACTION FLOW
--------------------------------------------------

Maintain proper RS485 direction handling.

For every transmission:

1. Set DE/RE HIGH.
2. Send Modbus frame.
3. Flush UART transmission.
4. Switch DE/RE LOW.
5. Wait for response.
6. Read response.
7. Validate CRC.
8. Decode basic Modbus frame.
9. Print raw frame.

Avoid leaving DE/RE HIGH while waiting for the slave response.

--------------------------------------------------
DEBUG OUTPUT
--------------------------------------------------

For every test, print enough information to diagnose the physical link.

Example:

====================================
MODBUS RTU TRANSACTION
====================================
Device: Invendis ET1641
Slave: 50
Baud: 9600
Format: 8N1
Function: 0x03
Address: 0x0000
Quantity: 1

TX:
32 03 00 00 00 01 XX XX

Waiting for response...

RX:
...

CRC:
OK / INVALID

Result:
SUCCESS / TIMEOUT / MODBUS EXCEPTION
====================================

Do not hide raw frames.

Raw TX/RX is critical for Phase 2 debugging.

--------------------------------------------------
ET1641 TEST PLAN
--------------------------------------------------

After implementation, the human operator will physically connect:

ESP32-C3
    |
MAX485
    |
RS485 A/B
    |
Invendis ET1641

ET1641 configuration:

Slave ID = 50
Baud = 9600
Parity = None
Stop = 1

First objective:

Get ANY valid response from slave ID 50.

Do not assume that a successful response must be a valid measurement.

There are three useful outcomes:

1. Normal Modbus response
   -> communication proven.

2. Modbus exception response
   -> communication with slave proven, but requested register/function may be invalid.

3. Timeout
   -> investigate physical wiring, polarity, direction control, slave ID, baud/configuration, power, etc.

--------------------------------------------------
TROUBLESHOOTING OUTPUT
--------------------------------------------------

For timeout, print:

Possible causes:

- ET1641 not powered
- RS485 A/B reversed
- incorrect slave ID
- incorrect baud
- incorrect parity
- incorrect stop bits
- DE/RE direction problem
- MAX485 wiring problem
- incorrect RS485 terminals
- physical cable problem

For CRC error:

ERROR:
Received frame has invalid CRC.

For Modbus exception:

Display:

Slave ID
Exception function
Exception code
Exception description if known

--------------------------------------------------
IMPORTANT RS485 BUS RULE
--------------------------------------------------

The architecture must support multiple devices on one RS485 bus.

Each device must have a unique Modbus slave ID.

Example:

DSE 7320 -> ID 10
ET1641 -> ID 50
Future device -> ID 51

Only one Modbus master should initiate transactions.

The ESP32-C3 is the Modbus master/client.

DSE and ET1641 are Modbus slaves/servers.

Do not implement simultaneous requests.

Use sequential polling/transactions in the future.

For this phase, manual CLI requests are sufficient.

--------------------------------------------------
DSE MUST REMAIN IN THE PROJECT
--------------------------------------------------

Do not rename the project to ET1641.

Do not replace DSE configuration.

Do not remove DSE-related documentation.

Do not remove DSE CLI functionality.

The project remains:

DSE 7320 Modbus RTU Monitoring Gateway

with ET1641 as the first additional supported Modbus device.

Future architecture:

DSE 7320
+
DC Energy Meter
+
additional RS485 devices
+
ESP32-C3 gateway

--------------------------------------------------
DOCUMENTATION
--------------------------------------------------

Update:

doc/status/phase_status.md

to include:

PHASE 2 — IN PROGRESS

Record:

- ET1641 added as first physical Modbus test device
- ET1641 configuration
- RS485 hardware configuration
- CLI changes
- current test status

Do not mark Phase 2 complete until a physical ET1641 response has actually been received and CRC validated.

Also update:

doc/spec/mvp.md

to explain that the MVP supports multiple Modbus device configurations while retaining DSE 7320 as the primary target.

Add/update:

doc/guide/getting_started.md

with the ET1641 physical test procedure.

--------------------------------------------------
DO NOT DO THESE THINGS
--------------------------------------------------

Do NOT:

- add MQTT
- add Wi-Fi
- add HTTP
- add cloud backend
- add database
- add OTA
- add dashboard
- add automatic register discovery
- guess register maps
- guess scaling
- replace DSE with ET1641
- introduce a third-party Modbus library
- rewrite working Phase 1 code unnecessarily
- add complicated persistence
- automatically poll many registers

Keep Phase 2 focused.

--------------------------------------------------
PHASE 2 SUCCESS CRITERIA
--------------------------------------------------

Phase 2 is complete only when:

[ ] ESP32-C3 communicates through MAX485
[ ] ET1641 is configured as slave ID 50
[ ] 9600 8N1 is working
[ ] RS485 TX frame is observed
[ ] ET1641 returns a response
[ ] RX frame is captured
[ ] CRC validates
[ ] Modbus response is decoded
[ ] CLI can select ET1641
[ ] DSE configuration remains intact
[ ] Multi-device architecture is in place
[ ] No register meanings are guessed
[ ] Phase status documentation is updated

At the end, the firmware should be ready for the human operator to connect the physical ET1641 and perform the first live test.

--------------------------------------------------
FINAL INSTRUCTION TO CURSOR
--------------------------------------------------

FIRST inspect the existing repository and understand the current Phase 1 implementation.

Do not blindly regenerate the project.

Make the smallest clean changes required to add multi-device configuration and ET1641 testing.

After implementation:

1. Build the project.
2. Report any compile errors.
3. Verify existing CLI functionality has not been broken.
4. Show the files changed.
5. Explain the new CLI commands.
6. Show the exact physical test procedure for the ET1641.
7. Do not claim RS485 communication is successful until a real ET1641 response is received.

## One architectural point I especially want Cursor to follow

Don't let it turn the project into:

```
ET1641 tester
```
It should become:

```
                    MODBUS RTU MASTER
                          │
                       ESP32-C3
                          │
                        MAX485
                          │
                      RS485 BUS
                          │
          ┌───────────────┼────────────────┐
          │               │                │
          ▼               ▼                ▼
      DSE 7320         ET1641          Future Device
      Generator        DCEM            RS485 Device
      Controller       Meter


```