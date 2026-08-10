# PHASE 2A — DETAILED RS485 / MODBUS RTU DIAGNOSTICS

# ESP32-C3 + MAX485 + Invendis ET1641

Project:
dse7320_rtu

Current phase:
Phase 2A — Physical RS485 diagnostic validation

IMPORTANT:
Do NOT move to register-map implementation yet.

Do NOT add MQTT, Wi-Fi, cloud, database, dashboard, OTA, automatic polling, or engineering-unit decoding.

The immediate objective is to perform a detailed diagnostic of the existing RS485/Modbus transaction implementation and determine whether the ESP32-C3 + MAX485 communication layer is behaving correctly.

---

## PROJECT CONTEXT

The project originally targets:

Primary devices:

- DSE 7320 MKII
- DSE 7320 EMF

Additional Modbus devices:

- Invendis ET1641 DC Energy Meter
- future RS485/Modbus devices

The ET1641 is currently being used as the first physical Modbus test device because the DSE controller is not currently available.

The ET1641 does NOT replace the DSE.

The firmware must remain a multi-device Modbus RTU architecture.

---

## CURRENT PHYSICAL DEVICE

Invendis:

Model:
ET1641

Device:
DC Energy Meter / DCEM

Serial Number:
725093

Modbus Slave ID:
50

Baud:
9600

Parity:
None

Stop bits:
1

Format:
8N1

Protocol:
Modbus RTU over RS485

---

## CURRENT ESP32-C3 HARDWARE

Board:

ESP32-C3 SuperMini

RS485 UART:

UART1

Pins:

RX = GPIO20
TX = GPIO21
DE/RE = GPIO4

MAX485:

DI -> GPIO21
RO -> GPIO20
DE -> GPIO4
RE -> GPIO4

DE and RE are tied together.

Direction:

GPIO4 HIGH = transmit
GPIO4 LOW = receive

The current physical MAX485 module is being powered from the ESP32-C3's 3.3V supply.

IMPORTANT:

Do NOT introduce a level shifter, voltage divider, different transceiver, or hardware redesign in this phase.

Do NOT change the current hardware configuration.

However, document the actual electrical configuration accurately.

Do not make unsupported claims about electrical safety or logic-level compatibility.

---

## CURRENT OBSERVATIONS

The firmware successfully generates Modbus requests.

Example FC03:

TX:
32 03 00 00 00 01 81 C9

Example FC04:

TX:
32 04 00 00 00 01 34 09

The user intentionally swapped RS485 A/B during some tests.

Therefore:

IMPORTANT:
The responses observed while A/B were intentionally swapped MUST NOT be interpreted as valid ET1641 Modbus responses unless their complete frame and CRC validate.

The purpose of those tests was specifically to observe the diagnostic behavior and CRC handling when the RS485 signal is incorrect.

Observed examples include:

RX:
32 84 02 3B 70

RX:
32 84 02 86 20

RX:
32 84 02 F7 4C

RX:
32 84 02 D3 31

RX:
32 84 02 08 10

RX:
32 84 02 92 F8

RX:
32 84 02 F8 16

RX:
32 84 02 56 FA

There were also periods of timeout.

Do NOT conclude from these frames that the ET1641 is definitely returning Modbus exception 0x02.

First validate the CRC and frame integrity.

---

## PRIMARY DIAGNOSTIC OBJECTIVE

Determine independently whether:

1. UART TX is correct.
2. UART TX completes before DE/RE changes.
3. DE/RE direction control is correct.
4. UART RX is enabled at the correct time.
5. RX buffer is clean before every transaction.
6. Received bytes are captured correctly.
7. Response timing is correct.
8. Frame length handling is correct.
9. Modbus CRC implementation is correct.
10. Modbus CRC validation is correct.
11. Modbus exception parsing is correct.
12. FC03 and FC04 requests are being handled independently.
13. No stale bytes from previous transactions are being reused.
14. No partial frames are being incorrectly accepted.
15. A/B polarity changes produce the expected diagnostic behavior.

Do not assume the physical device is the problem.

Do not assume the firmware is the problem.

Collect evidence for each layer.

---

## FIRST: INSPECT THE EXISTING CODE

Before modifying anything, inspect:

src/main.cpp

src/config/config.h

src/rs485/rs485.h
src/rs485/rs485.cpp

src/modbus/modbus_crc.h
src/modbus/modbus_crc.cpp

src/modbus/modbus_client.h
src/modbus/modbus_client.cpp

src/cli/cli.h
src/cli/cli.cpp

Do not rewrite working code unnecessarily.

Identify exactly:

- how UART1 is initialized
- how UART parameters are configured
- how DE/RE is controlled
- how TX bytes are written
- how TX completion is detected
- whether Serial.flush() is used
- when DE/RE changes from TX to RX
- how the RX buffer is cleared
- how response bytes are collected
- how timeout is implemented
- how expected response length is determined
- how CRC is calculated
- how CRC is compared
- how Modbus exceptions are detected
- how FC03 and FC04 are parsed
- whether command input can interfere with the RS485 UART
- whether the same serial object is accidentally used for USB CDC and RS485

Report these findings before making major changes.

---

## DIAGNOSTIC 1 — CRC SELF TEST

Create an internal CRC self-test.

This test must NOT require the ET1641.

Verify the Modbus CRC implementation against known frames.

Known request:

32 03 00 00 00 01

Expected Modbus CRC:

CRC = 0xC981

Wire order:

81 C9

Therefore:

32 03 00 00 00 01 81 C9

must validate.

Second request:

32 04 00 00 00 01

Expected CRC:

0x0934

Wire order:

34 09

Therefore:

32 04 00 00 00 01 34 09

must validate.

Also test:

32 84 02

Expected CRC:

0xCE32

Wire order:

32 CE

Therefore a valid Modbus exception frame would be:

32 84 02 32 CE

The CRC implementation must calculate and validate these correctly.

Add a CLI command if appropriate:

crc-test

Expected output:

## CRC SELF TEST

FC03 request: PASS
FC04 request: PASS
FC04 exception frame: PASS

If any test fails, STOP physical debugging and fix CRC first.

---

## DIAGNOSTIC 2 — TRANSACTION STATE MACHINE

Instrument the RS485 transaction with explicit state logging.

Use states such as:

IDLE
PREPARE_TX
TX_ACTIVE
TX_COMPLETE
SWITCH_TO_RX
RX_WAIT
RX_RECEIVING
RX_COMPLETE
VALIDATE
DONE
ERROR

Example:

[MODBUS] IDLE
[MODBUS] RX buffer cleared
[MODBUS] DE/RE = HIGH
[MODBUS] TX start
[MODBUS] TX complete
[MODBUS] DE/RE = LOW
[MODBUS] RX wait
[MODBUS] RX complete
[MODBUS] CRC validation
[MODBUS] DONE

This is diagnostic mode and can later be disabled.

---

## DIAGNOSTIC 3 — TX COMPLETION

Verify that the firmware does NOT switch DE/RE to receive before the final UART byte has physically left the UART.

The sequence must be:

DE/RE HIGH
    ↓
write complete Modbus frame
    ↓
wait for UART transmission completion
    ↓
DE/RE LOW
    ↓
wait for response

Do NOT simply assume that writing bytes to the UART buffer means transmission has completed.

Log:

TX start timestamp
TX complete timestamp
DE HIGH timestamp
DE LOW timestamp

Example:

TX start:       123456 us
TX complete:    123589 us
DE -> RX:       123600 us

The exact values are not important; the ordering is.

---

## DIAGNOSTIC 4 — RX BUFFER CLEANUP

Before every Modbus transaction:

1. Ensure no stale bytes remain in the UART RX buffer.
2. Clear/drain stale data.
3. Log how many bytes were discarded.

Example:

RX buffer before transaction: 0 bytes

If stale bytes exist:

WARNING:
Discarding 3 stale RX bytes:
XX XX XX

Do NOT silently discard them.

This is important because a previous invalid response could otherwise become the next transaction's response.

---

## DIAGNOSTIC 5 — RAW BYTE TIMING

During diagnostic mode, record:

- timestamp of first RX byte
- timestamp of every RX byte if practical
- total RX length
- time between bytes
- time from DE LOW to first RX byte
- total response duration

Example:

RX start: +4.2 ms
RX byte 0: 32
RX byte 1: 84
RX byte 2: 02
RX byte 3: 3B
RX byte 4: 70

RX length: 5

This will help determine whether the data is a coherent frame or noise/corruption.

---

## DIAGNOSTIC 6 — STRICT MODBUS FRAME VALIDATION

Never classify a received frame as valid merely because:

- slave ID matches
- function looks correct
- frame has enough bytes

A frame must pass ALL relevant checks.

For normal FC03/FC04:

1. minimum length
2. slave ID
3. function code
4. byte count
5. expected total length
6. CRC

For exception:

1. slave ID
2. function code has MSB set
3. exception function corresponds to the requested function
4. exception code is present
5. CRC validates

Example:

Request:

32 04 00 00 00 01 34 09

Valid exception:

32 84 02 32 CE

Meaning:

Slave = 50
Exception function = 0x84
Original function = 0x04
Exception code = 0x02
CRC = valid

But:

32 84 02 3B 70

MUST be classified as:

INVALID FRAME / CRC ERROR

unless the calculated CRC actually matches.

Do not report it as a valid Modbus exception.

---

## DIAGNOSTIC 7 — CRC DEBUG OUTPUT

For every received frame, show:

RX:
32 84 02 3B 70

CRC received:
0x703B

CRC calculated:
0xCE32

CRC:
INVALID

Result:
INVALID MODBUS FRAME

IMPORTANT:

Modbus CRC is transmitted low byte first.

Make sure the implementation compares the received bytes in the correct order.

Do not reverse the CRC incorrectly.

---

## DIAGNOSTIC 8 — FC03 / FC04 INDEPENDENCE

Verify that a request using FC03 can only be interpreted against an FC03 response.

Example:

Request:

32 03 00 00 00 01 81 C9

A response:

32 84 02 ...

must NOT be treated as a valid response to the FC03 request.

Expected exception function for FC03:

0x83

Expected exception function for FC04:

0x84

Therefore:

# FC03 request + 0x84 response

unexpected function response

# FC04 request + 0x84 response

potential exception response, pending CRC validation

Add explicit diagnostics for this.

---

## DIAGNOSTIC 9 — TRANSACTION ID / REQUEST CORRELATION

For every transaction, internally record:

request slave
request function
request address
request quantity

Then validate the response against those values.

Example:

REQUEST:
slave=50
function=04
address=0000
quantity=1

RESPONSE:
slave=50
function=84
exception=02

This may be a matching exception response IF CRC validates.

But:

REQUEST:
function=03

RESPONSE:
function=84

must be reported as:

UNEXPECTED FUNCTION RESPONSE

Do not accept it.

---

## DIAGNOSTIC 10 — TIMEOUT CLASSIFICATION

Improve timeout diagnostics.

Instead of only:

ERROR: Modbus timeout

report:

MODBUS TIMEOUT

TX sent: YES
TX completed: YES
DE/RE switched to RX: YES
RX bytes received: 0
Elapsed: XXXX ms

Possible causes:

- slave not responding
- wrong slave ID
- wrong baud
- wrong parity
- wrong stop bits
- A/B polarity
- RS485 wiring
- device not powered
- DE/RE timing
- transceiver issue

If 1 or more bytes were received before timeout:

MODBUS PARTIAL RESPONSE

RX length: X

RX:
...

Do not classify partial data as a clean timeout.

---

## DIAGNOSTIC 11 — A/B POLARITY TEST

Document the physical test procedure.

Test A:

Correct presumed A/B wiring.

Run:

rawread 50 04 0000 1

Test B:

Intentionally swap A/B.

Run exactly the same command.

Record both outputs.

Do not interpret either response as valid until CRC passes.

Expected diagnostic principle:

Correct polarity should produce either:

- valid normal Modbus response
- valid Modbus exception

depending on whether the requested register is valid.

Incorrect polarity should normally produce:

- timeout
- no response
- corrupted/invalid bytes
- invalid CRC
- other non-deterministic reception

A response with a recognizable-looking prefix but invalid CRC must be classified as invalid.

---

## DIAGNOSTIC 12 — REPEATABILITY TEST

Add an optional command:

repeat     

Example:

repeat 20 50 04 0000 1

Do NOT use this to discover registers.

This is only to test communication repeatability.

Output summary:

## MODBUS REPEAT TEST

Requests: 20
Valid responses: X
Valid exceptions: X
CRC errors: X
Partial frames: X
Timeouts: X
Unexpected function: X
Other errors: X

This will help determine whether the physical link is stable.

Do not poll continuously by default.

---

## DIAGNOSTIC 13 — TERMINAL INPUT

The logs showed terminal escape sequences such as:

[A[B
[D[D[D

These appear to be terminal cursor/arrow-key escape sequences.

Investigate whether USB CLI input is accidentally being mixed with RS485 data.

Confirm:

USB CDC / CLI uses its own serial interface.

RS485 UART1 is independent.

No RS485 RX bytes should be interpreted as CLI characters.

No CLI input should be transmitted through the RS485 UART.

If necessary, improve CLI handling so unknown ANSI escape sequences are ignored cleanly.

This is a secondary issue and must NOT interfere with Modbus diagnostics.

---

## DIAGNOSTIC 14 — NO REGISTER ASSUMPTIONS

Do NOT assume:

0x0000

is a valid ET1641 register.

We currently do not have a verified ET1641 register map.

The purpose of querying 0x0000 is only to test the communication path.

A valid Modbus exception such as:

32 84 02 

would actually be useful because it proves the slave understood the request and rejected the address.

But this conclusion may only be made AFTER CRC validation.

---

## DIAGNOSTIC 15 — DO NOT CHANGE DEVICE CONFIGURATION

Keep:

DSE 7320 configuration.

ET1641 configuration:

Device:
Invendis ET1641

Serial:
725093

Slave:
50

Baud:
9600

Parity:
None

Stop:
1

Do not replace the DSE defaults with ET1641 settings.

The selected-device architecture must remain.

---

## DIAGNOSTIC 16 — CONFIG COMMAND

Make sure:

config

clearly reports the currently selected device and UART settings.

Example:

## CURRENT DEVICE

Name: Invendis ET1641
Model: ET1641
Serial: 725093

## MODBUS

Slave ID: 50
Baud: 9600
Parity: None
Stop bits: 1
Function: 04

## RS485

UART: UART1
RX: GPIO20
TX: GPIO21
DE/RE: GPIO4

---

## DIAGNOSTIC 17 — PHYSICAL TEST MODE

Add a clearly documented diagnostic mode if necessary.

Example:

diagnostic on
diagnostic off

When diagnostic mode is ON, print detailed transaction information.

Normal mode should remain concise.

Do not make the normal firmware unusably verbose.

---

## IMPORTANT: DO NOT MODIFY THE HARDWARE

The current test hardware is:

ESP32-C3 SuperMini
+
MAX485 module
+
Invendis ET1641

The MAX485 is currently powered from the ESP32-C3's 3.3V supply.

Do NOT:

- add voltage dividers
- add level shifters
- replace MAX485
- change GPIO assignments
- change supply voltage
- redesign the RS485 circuit

unless explicitly requested later.

This task is firmware/protocol diagnostics.

However, document the hardware configuration in the test documentation.

---

## IMPORTANT: DO NOT ADD A MODBUS LIBRARY

Continue using the existing custom Modbus RTU implementation.

We need full visibility into:

- CRC
- UART
- DE/RE
- frame parsing
- timing
- RX buffering

A third-party library would hide too much at this diagnostic stage.

---

## EXPECTED DIAGNOSTIC OUTPUT

For:

rawread 50 04 0000 1

the output should look approximately like:

# ========================================
MODBUS RTU DIAGNOSTIC

Device: Invendis ET1641
Serial: 725093

Slave: 50
Baud: 9600
Format: 8N1

Function: 0x04
Address: 0x0000
Quantity: 1

[1] Clear RX buffer
RX stale bytes: 0

[2] DE/RE -> TX

[3] TX
32 04 00 00 00 01 34 09

[4] TX complete

[5] DE/RE -> RX

[6] Waiting for response...

[7] RX
32 84 02 XX XX

RX length: 5

[8] Frame validation

Slave:
50 OK

Function:
0x84

Expected:
0x84

Exception:
0x02

CRC received:
0xXXXX

CRC calculated:
0xXXXX

CRC:
VALID / INVALID

RESULT:
VALID MODBUS EXCEPTION
or
INVALID MODBUS FRAME

========================================

---

## FINAL ENGINEERING REQUIREMENT

Do not "fix" the system by guessing.

Do not change multiple variables simultaneously.

The diagnostic process must isolate:

1. CRC implementation
2. UART transmission
3. RS485 direction control
4. RX buffering
5. response timing
6. frame parsing
7. physical A/B polarity
8. device configuration

After implementation:

1. Build successfully.
2. Run crc-test.
3. Run a single FC04 test.
4. Run a single FC03 test.
5. Run A/B correct-polarity test.
6. Run A/B reversed-polarity test.
7. Run repeatability test if available.
8. Report exact TX/RX frames.
9. Report CRC calculated vs received.
10. Report whether each response is valid, invalid, partial, or timeout.

Do NOT declare Phase 2 complete yet.

Phase 2 is complete only after at least one physically connected ET1641 transaction produces a CRC-valid Modbus frame.

---

## CURSOR RESPONSE FORMAT

After making the changes, report:

1. Root causes found in the existing implementation.
2. Files changed.
3. Exact changes made.
4. CRC self-test results.
5. Build result.
6. New diagnostic commands.
7. Expected physical test procedure.
8. What output should be captured and sent back for analysis.

Do not fabricate successful communication.

Only claim physical Modbus communication is proven when a real ET1641 response has been received and its CRC has been independently validated.