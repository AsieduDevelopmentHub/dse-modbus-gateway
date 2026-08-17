# FULL SYSTEM DIAGNOSTIC AUDIT
# ET1641 DCEM + ESP32-C3 + MAX485 + Modbus RTU
#
# Project: dse-modbus-gateway
# Current Phase: ET1641 integration diagnostic audit
#
# IMPORTANT:
# Do NOT assume the CRC algorithm is wrong.
# Do NOT assume the ET1641 is faulty.
# Do NOT assume the MAX485 is faulty.
#
# We have already performed the basic tests and need a complete
# code/algorithm/architecture audit to determine where the problem is.

============================================================
1. CURRENT SYSTEM
============================================================

MCU:
ESP32-C3 SuperMini

Framework:
Arduino / PlatformIO

RS485:
MAX485 module

RS485 UART:
UART1

Pins:
RX  = GPIO20
TX  = GPIO21
DE/RE = GPIO4

DE/RE:
HIGH = transmit
LOW  = receive

USB:
ESP32-C3 native USB CDC

Current Modbus settings for test device:

Baud:
9600

Format:
8N1

Protocol:
Modbus RTU

============================================================
2. TEST DEVICE
============================================================

Device:
Invendis ET1641 DC Energy Meter

Device type:
DCEM

Slave ID:
50

Serial number:
725093

Power:
20–60 V DC

Parity:
None

Stop bits:
1

The ET1641 is powered independently from its required DC supply.

ESP32-C3 + MAX485 are powered from USB.

For the current bench test:

ET1641 RS485/common reference is connected to the
MAX485/ESP32 GND.

RS485:
ET1641 A -> MAX485 A
ET1641 B -> MAX485 B

The physical wiring has been checked.

IMPORTANT:
Two separate ET1641 devices have been tested.

Both devices produce the same class of CRC-invalid response.

Therefore, do NOT immediately conclude that either individual
ET1641 is faulty.

============================================================
3. CURRENT OBSERVED TRANSACTION
============================================================

Command:

rawread 50 04 0000 1

TX:

32 04 00 00 00 01 34 09

This TX frame is expected to be:

Slave:
0x32 = decimal 50

Function:
0x04

Address:
0x0000

Quantity:
0x0001

CRC:
34 09

The firmware's internal CRC self-test PASSES.

Current RX example:

32 84 02 08 10

RX length:
5

Firmware reports:

CRC received:
0x1008

CRC calculated:
0xCE32

CRC:
INVALID

The RX therefore has the shape:

32 84 02 XX XX

where:

32 = slave 50
84 = 0x04 + 0x80
02 = Modbus exception code 02

However:

DO NOT classify this as a valid Modbus exception unless the CRC
is valid.

A valid frame for:

32 84 02

would need the correct CRC bytes.

============================================================
4. IMPORTANT PREVIOUS TEST INFORMATION
============================================================

A/B polarity was intentionally reversed during earlier tests
to observe corrupted/exception-like traffic.

Therefore, earlier invalid responses must not be interpreted as
proof that the ET1641 accepted the request.

The current investigation must distinguish:

- valid Modbus response
- valid Modbus exception
- corrupted frame
- partial frame
- timeout
- stale RX data
- unexpected function
- wrong transaction
- physical noise

============================================================
5. CRC SELF-TEST
============================================================

The CRC implementation has already passed the internal CRC test.

Known request:

32 03 00 00 00 01 81 C9

Known request:

32 04 00 00 00 01 34 09

Known exception prefix:

32 84 02

The CRC implementation passes these self-tests.

DO NOT simply replace the CRC algorithm without proving it is wrong.

Instead, audit every place where CRC is:

1. calculated
2. appended
3. transmitted
4. received
5. extracted
6. byte-order interpreted
7. validated

============================================================
6. FULL CODE AUDIT
============================================================

Inspect the entire repository before changing code.

Especially inspect:

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

Also inspect:

platformio.ini

and ALL configuration/device-definition files.

Do not only inspect the Modbus CRC file.

Trace the complete execution path:

CLI command
    ↓
command parser
    ↓
device selection
    ↓
device configuration
    ↓
Modbus request creation
    ↓
CRC generation
    ↓
RS485 TX
    ↓
UART transmission
    ↓
DE/RE transition
    ↓
RS485 RX
    ↓
UART buffer
    ↓
frame collection
    ↓
frame parser
    ↓
CRC extraction
    ↓
CRC validation
    ↓
Modbus response classification
    ↓
CLI output

Document this complete path.

============================================================
7. DEVICE CONFIGURATION AUDIT
============================================================

Verify that selecting:

device select 1

actually changes the runtime configuration.

The selected ET1641 configuration must be:

Name:
Invendis ET1641

Model:
ET1641

Serial:
725093

Key:
et1641

Slave:
50

Baud:
9600

Parity:
None

Stop:
1

Format:
8N1

Verify that rawread actually uses this runtime configuration.

Do NOT rely only on config.h.

Print the actual runtime values used by the Modbus transaction.

============================================================
8. CRITICAL CLI AUDIT
============================================================

There is evidence of a separate command-input problem.

Example:

User entered:

rawread 50 03 0000 1

but output appeared as:

rawre0 04 0000 3

and previous logs contained:

[A[B
[D[D[D

This suggests possible:

- ANSI escape sequence handling
- USB CDC input corruption
- CR/LF processing
- local echo interference
- command-buffer reuse
- command-buffer overflow
- character loss
- stale command data
- parser/tokenization issue

Audit the CLI completely.

Verify that:

USB CDC input

is completely independent from:

UART1 RS485 input.

A character received from RS485 must NEVER enter the CLI parser.

A CLI character must NEVER be transmitted through UART1 unintentionally.

Test:

rawread 50 03 0000 1

and verify the exact command string received by the parser.

Add debug output showing:

RAW COMMAND:
...

TOKEN 0:
...

TOKEN 1:
...

TOKEN 2:
...

TOKEN 3:
...

TOKEN 4:
...

PARSED:
slave=50
function=03
address=0000
quantity=1

Do this temporarily in diagnostic mode.

============================================================
9. REQUEST CONSTRUCTION AUDIT
============================================================

For every request verify:

slave byte
function byte
address high byte
address low byte
quantity high byte
quantity low byte
CRC low byte
CRC high byte

For:

slave 50
function 04
address 0000
quantity 1

the final request MUST be:

32 04 00 00 00 01 34 09

Print both:

logical request fields

and

final byte array.

Verify that the exact byte array passed to UART transmission
is the same byte array shown in the debug output.

No hidden modification after logging.

============================================================
10. TX UART AUDIT
============================================================

Inspect exactly how bytes are transmitted.

Verify:

- UART1 is actually used
- correct baud
- correct data bits
- correct parity
- correct stop bits
- correct TX GPIO
- correct RX GPIO

Do not assume configuration based on source code.

Print runtime UART configuration.

Verify whether:

Serial.write()

or

HardwareSerial.write()

is being used correctly.

Verify that the number of bytes written equals the expected
request length.

============================================================
11. UART TX COMPLETION AUDIT
============================================================

This is CRITICAL.

Verify that DE/RE does NOT switch to RX until the final byte
has actually left the UART hardware.

Correct conceptual sequence:

DE/RE = HIGH
    ↓
write request
    ↓
wait for UART hardware TX completion
    ↓
DE/RE = LOW
    ↓
receive response

Do NOT treat:

write()

returning

as equivalent to:

physical transmission complete.

Audit:

flush()
UART FIFO
UART TX-empty state
hardware transmission-complete behavior

and the actual ESP32-C3 Arduino HardwareSerial implementation.

Use the correct mechanism for this MCU/framework.

Document what mechanism is being used and why.

============================================================
12. DE/RE AUDIT
============================================================

Verify:

GPIO4 HIGH:
TX mode

GPIO4 LOW:
RX mode

Audit whether:

DE
and
RE

are actually tied together as expected.

Print:

DE/RE before TX
DE/RE during TX
DE/RE after TX

Verify GPIO initialization order.

Verify no other code changes GPIO4.

Search the entire repository for:

GPIO4

and:

DE
RE

to ensure there is no conflicting ownership.

============================================================
13. RX BUFFER AUDIT
============================================================

Before every transaction:

1. clear/drain RX buffer
2. count discarded bytes
3. log discarded bytes
4. begin transaction with known-empty RX state

After TX completes:

switch to RX

Then capture ALL received bytes.

Do not:

- silently discard bytes
- stop early without a reason
- reuse old buffer contents
- parse before the complete frame is available
- overwrite bytes accidentally

Audit:

buffer allocation
buffer size
index handling
overflow behavior
reset behavior
timeout behavior

============================================================
14. RX TIMING AUDIT
============================================================

Instrument:

transaction start
TX start
TX complete
DE -> RX
first RX byte
each RX byte if practical
last RX byte
frame completion
timeout

Example:

TX START:       ...
TX COMPLETE:    ...
DE -> RX:       ...
RX BYTE 0:      ...
RX BYTE 1:      ...
RX BYTE 2:      ...
RX BYTE 3:      ...
RX BYTE 4:      ...
RX COMPLETE:    ...

Determine whether response timing is consistent.

============================================================
15. MODBUS RTU FRAME BOUNDARY AUDIT
============================================================

Do not assume that:

"5 bytes arrived"

automatically means:

"complete valid exception frame."

Implement robust frame boundary logic.

For an exception:

expected minimum frame:

5 bytes

But still validate:

slave
function
exception code
CRC

For normal FC03/FC04 responses:

validate:

slave
function
byte count
expected total frame length
CRC

Investigate whether the receiver is terminating the frame too early
or too late.

============================================================
16. CRC RECEIVER AUDIT
============================================================

For every RX frame print:

RAW RX:
...

PAYLOAD:
...

RECEIVED CRC LOW:
...

RECEIVED CRC HIGH:
...

RECEIVED CRC COMBINED:
...

CALCULATED CRC:
...

CRC RESULT:
PASS / FAIL

Explicitly verify that the received bytes are combined as:

CRC = low_byte | (high_byte << 8)

if that is the format used internally.

Verify no accidental reversal occurs.

Also verify the CRC calculation excludes the received CRC bytes.

============================================================
17. RESPONSE FUNCTION AUDIT
============================================================

For request FC04:

Normal response:
0x04

Exception response:
0x84

For request FC03:

Normal response:
0x03

Exception response:
0x83

Verify the parser does not accept:

FC03 request + FC84 response

as a valid exception.

The response must correspond to the request.

============================================================
18. EXCEPTION RESPONSE AUDIT
============================================================

A valid exception response is:

slave
function | 0x80
exception code
CRC

For:

32 84 02

the frame is only valid if the CRC matches.

If CRC fails:

RESULT:
INVALID FRAME / CRC ERROR

NOT:

VALID MODBUS EXCEPTION

============================================================
19. STALE RESPONSE AUDIT
============================================================

Investigate whether:

Transaction N response

could be consumed by:

Transaction N+1.

Add a transaction sequence number:

TXN #001
TXN #002
TXN #003

For each transaction print:

TXN ID
request
response

This will help identify stale-buffer behavior.

============================================================
20. MULTIPLE TRANSACTION AUDIT
============================================================

Test:

FC03
FC04

separately.

Do not run them continuously yet.

Verify:

FC03 request always results in an FC03-related response
or timeout.

FC04 request always results in an FC04-related response
or timeout.

Do not accept a response from a previous transaction.

============================================================
21. RAW UART TEST
============================================================

Implement a temporary diagnostic command:

rs485raw <slave> <function> <address> <quantity>

It must:

- create the request
- transmit it
- capture raw bytes
- print raw bytes only
- not parse them
- not modify them
- not calculate CRC before displaying them

Example:

TX:
32 04 00 00 00 01 34 09

RX:
32 84 02 08 10

RX LENGTH:
5

Then separately calculate/validate CRC.

This allows us to distinguish:

RAW UART PROBLEM

from:

MODBUS PARSER PROBLEM.

============================================================
22. REPEATABILITY TEST
============================================================

Implement:

rs485raw-repeat 20 50 04 0000 1

For each transaction report:

TX
RX
length
timing

Summary:

20 attempts

0-byte timeout:
X

5-byte response:
X

other-length response:
X

CRC valid:
X

CRC invalid:
X

This is for diagnostics only.

Do not turn it into automatic polling.

============================================================
23. DEVICE-SIDE ASSUMPTIONS
============================================================

Do NOT assume address 0x0000 is valid.

The purpose of the current test is communication validation.

The ET1641 register map must not be guessed.

If the device responds with a CRC-valid exception 0x02,
that is useful evidence that the communication layer works.

If the device responds with CRC-invalid bytes,
do not interpret the exception code as authoritative.

============================================================
24. HARDWARE CONFIGURATION
============================================================

Do NOT modify the hardware during this audit.

Current setup:

ET1641:
20–60 V DC power

ESP32-C3:
USB power

MAX485:
currently powered from ESP32-C3 3.3 V

Common reference:
ET1641 communication ground connected to MAX485/ESP32 ground

RS485:
A -> A
B -> B

Do not add:

- level shifter
- voltage divider
- another transceiver
- external termination
- different GPIOs

unless the audit identifies a specific reason.

If an electrical concern is discovered, report it separately as:

HARDWARE RISK

Do not silently change the design.

============================================================
25. SEARCH FOR HIDDEN BUGS
============================================================

Perform repository-wide searches for:

Serial
HardwareSerial
Serial1
UART1
GPIO20
GPIO21
GPIO4
DE
RE
flush
available
read
write
CRC
crc
modbus
buffer
timeout
millis
micros
delay

Look for:

- duplicate UART initialization
- multiple ownership of GPIOs
- global buffer reuse
- static buffer corruption
- race conditions
- incorrect signed/unsigned conversions
- integer truncation
- address parsing bugs
- quantity parsing bugs
- decimal/hex confusion
- incorrect byte ordering
- incorrect CRC byte ordering
- response buffer overflow
- response buffer underflow
- timeout logic errors
- state-machine errors
- parser errors

============================================================
26. PLATFORMIO / ESP32-C3 AUDIT
============================================================

Inspect platformio.ini.

Verify:

board
framework
USB CDC configuration
monitor configuration
UART configuration
build flags

Ensure the diagnostic implementation is actually appropriate
for ESP32-C3 Arduino/PlatformIO.

Do not use ESP32 APIs intended for another ESP32 variant without
checking compatibility.

============================================================
27. DEVICE ABSTRACTION AUDIT
============================================================

The project must NOT become ET1641-only.

The architecture should support:

DSE 7320 MKII
DSE 7320 EMF
Invendis ET1641
future RS485 devices

The common layers should remain:

Device configuration
      ↓
Modbus RTU transport
      ↓
RS485 driver
      ↓
UART

The ET1641 integration should only provide device-specific
configuration/register definitions later.

Do not hard-code ET1641 assumptions into the generic Modbus
transport layer.

============================================================
28. DO NOT "FIX" WITHOUT EVIDENCE
============================================================

For every proposed code change:

Explain:

CURRENT BEHAVIOR
ROOT CAUSE
WHY IT IS WRONG
CHANGE
EXPECTED RESULT

Do not perform random timing changes.

Do not randomly add delays.

Do not change baud rates.

Do not change CRC algorithm.

Do not swap A/B in software.

Do not change slave ID.

Do not change register addresses.

unless evidence supports the change.

============================================================
29. TEST MATRIX
============================================================

After the audit/fixes, prepare this matrix:

TEST 1:
CRC self-test

Expected:
PASS

TEST 2:
CLI parser test

Expected:
exact command parsed

TEST 3:
TX frame test

Expected:
32 04 00 00 00 01 34 09

TEST 4:
RX raw capture

Expected:
deterministic raw bytes

TEST 5:
CRC validation

Expected:
correct PASS/FAIL

TEST 6:
FC03

Expected:
FC03-related response or timeout

TEST 7:
FC04

Expected:
FC04-related response or timeout

TEST 8:
A/B correct

Record result.

TEST 9:
A/B reversed

Record result.

TEST 10:
20-repeat test

Record:
timeouts
partial frames
CRC failures
valid frames

============================================================
30. FINAL REPORT REQUIRED
============================================================

Do NOT simply say:

"CRC is invalid."

Produce a complete engineering report:

A. Architecture audit
B. CLI audit
C. Device configuration audit
D. Request construction audit
E. CRC algorithm audit
F. UART configuration audit
G. TX audit
H. DE/RE audit
I. RX audit
J. RX buffer audit
K. Timing audit
L. Frame-boundary audit
M. Response parser audit
N. Exception parser audit
O. Device abstraction audit
P. PlatformIO/ESP32-C3 audit
Q. Hardware-risk observations

For every problem found:

SEVERITY:
CRITICAL / HIGH / MEDIUM / LOW

ROOT CAUSE:
...

EVIDENCE:
...

FIX:
...

VERIFICATION:
...

============================================================
31. SUCCESS CRITERIA
============================================================

Do NOT declare the ET1641 integration successful merely because:

- bytes are received
- slave ID matches
- function appears correct
- exception code appears correct

Communication is considered PROVEN only when:

1. request is correctly constructed
2. request is physically transmitted
3. DE/RE transitions correctly
4. response is captured without corruption
5. frame boundaries are correct
6. response corresponds to the request
7. CRC validates

Only then can we proceed to the ET1641 register map.

============================================================
32. FINAL CURSOR RESPONSE
============================================================

After completing the audit, report:

1. Files inspected
2. Files modified
3. Bugs found
4. Root cause of each bug
5. Code changes
6. CRC test result
7. CLI test result
8. TX test result
9. RX test result
10. DE/RE result
11. timing result
12. raw-frame result
13. repeatability result
14. remaining hardware risks
15. remaining unknowns
16. exact commands for the next physical test

IMPORTANT:

Do not fabricate physical test results.

Do not say RS485 communication is working unless the
received frame has a valid CRC.

Do not say the ET1641 is faulty unless there is evidence
supporting that conclusion.

The purpose of this task is to find the actual root cause,
not to force the system to report success.