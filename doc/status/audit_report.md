# Full System Diagnostic Audit Report

**Date:** 2026-08-11  
**Project:** `dse7320_rtu`  
**Phase:** ET1641 integration diagnostic audit  

**Physical Modbus success:** NOT claimed (no CRC-valid frame yet).

---

## A. Architecture audit

```text
CLI (USB CDC Serial)
  → device profile (compile-time table)
  → ModbusClient (request build + CRC + parse)
  → Rs485Bus (DE/RE + UART1 HardwareSerial)
  → MAX485 → RS485 bus → ET1641 / DSE / future
```

Multi-device abstraction remains (DSE + ET1641). Transport is generic.

## B. CLI audit

| Item | Finding |
|------|---------|
| USB vs UART1 | Separate — CLI uses `Serial` (USB CDC); RS485 uses `HardwareSerial(1)` |
| Escape sequences | Partially filtered; arrow keys can still produce junk if ESC lost |
| Dual echo | **MEDIUM** — `monitor_echo=yes` + firmware echo caused interleaved/corrupt-looking commands in logs |
| Fix | `monitor_echo=no`; keep firmware echo; diagnostic mode prints `RAW COMMAND` / tokens |

## C. Device configuration audit

`device select et1641` / `1` applies slave 50, 9600 8N1 via `rs485.applyConfig()`.  
`rawread` uses explicit slave/FC args; UART baud from runtime `_baudRate`.  
Txn banner now prints **runtime UART** and **FINAL TX BYTE ARRAY**.

## D. Request construction audit

For `rawread 50 04 0000 1` firmware builds:

`32 04 00 00 00 01 34 09` — matches known-good vector.

## E. CRC algorithm audit

| Vector | Result |
|--------|--------|
| `32 03 00 00 00 01 81 C9` | PASS |
| `32 04 00 00 00 01 34 09` | PASS |
| `32 84 02 32 CE` | PASS |

Byte order: Lo then Hi; validate with `low \| (high<<8)`. **CRC code is not the root cause.**

## F. UART configuration audit

UART1, GPIO20 RX / GPIO21 TX, 9600 8N1 from device profile / `set baud`.

**HARDWARE RISK:** GPIO20/21 are also ESP32-C3 default UART0 pins. With USB CDC, UART1 remaps them — usually OK, but pin conflict is a residual risk if UART0 is not fully released.

## G. TX audit

`write()` + `flush()` + **`uart_wait_tx_done(UART1)`** + settle, then DE→RX.  
Logs `write() returned` vs expected length.

## H. DE/RE audit

Only `RS485_DE_RE_PIN` (GPIO4) drives direction. HIGH=TX, LOW=RX. No other owners found in `src/`.

## I–L. RX / buffer / timing / frame boundary

- Stale RX counted before TX  
- Silence-based frame end (~5 ms)  
- First RX typically ~11–12 ms after wait (repeatable)  
- Always capturing **5 bytes**: `32 84 02 XX XX` with **CRC FAIL**  
- Parser correctly refuses to call this a valid exception  

## M–N. Response / exception parser

FC03 expects exception `0x83`; FC04 expects `0x84`.  
Observed `0x84` even on FC03 requests **with invalid CRC** → classified invalid / unexpected, not success.

## O. Device abstraction

ET1641 not hard-coded into CRC/transport. Profiles in `devices/modbus_device.cpp`.

## P. PlatformIO / ESP32-C3

`esp32-c3-devkitm-1`, USB CDC on boot, RTS/DTR off — appropriate for SuperMini.

## Q. Hardware-risk observations

| Risk | Severity | Notes |
|------|----------|-------|
| MAX485 on **3.3V** | **HIGH** | Many MAX485 modules are marginal at 3.3V; fits “stable prefix + random CRC” |
| Two ET1641s, same CRC fail | — | Points away from a single bad meter |
| Common GND present (per user) | — | Good; still verify continuity |
| No termination (short bench) | LOW | Usually OK on short wires |

**Do not silently redesign hardware in this audit.** Changing transceiver/supply is a separate, explicit step.

---

## Bugs found

### 1. CLI dual-echo / input interleaving  
**SEVERITY:** MEDIUM  
**ROOT CAUSE:** Host `monitor_echo=yes` + firmware echo; typing during TX interleaved with logs.  
**FIX:** `monitor_echo=no`; diagnostic `RAW COMMAND` dump.  
**VERIFICATION:** Clean single-character echo; `diagnostic on` shows exact parse.

### 2. TX completion relied only on `flush()`  
**SEVERITY:** LOW–MEDIUM (hardening)  
**ROOT CAUSE:** Audit requires hardware TX-empty certainty on ESP32-C3.  
**FIX:** Add `uart_wait_tx_done(UART_NUM_1)` after `flush()`.  
**VERIFICATION:** TX timing logs still show complete-before-DE-low; CRC situation unchanged unless this was contributing (unlikely given 11 ms RX delay).

### 3. No raw-UART bypass path  
**SEVERITY:** MEDIUM (diagnostic gap)  
**ROOT CAUSE:** Could not separate UART capture from Modbus parser.  
**FIX:** `rs485raw` / `rs485raw-repeat`.  
**VERIFICATION:** Raw RX dump + separate CRC audit on same bytes.

---

## Root cause conclusion (current evidence)

| Layer | Status |
|-------|--------|
| CRC math | OK |
| Request bytes | OK |
| DE/RE sequencing | OK (instrumented) |
| Parser classification | OK (rejects bad CRC) |
| Physical RX integrity | **FAILING** — CRC bytes never match `32 CE` |

**Most probable:** electrical/transceiver RX path (MAX485 @ 3.3V / signal integrity), **not** ET1641 uniqueness and **not** CRC algorithm.

---

## New commands (next physical test)

```text
diagnostic on
crc-test
device select et1641
config
rs485raw 50 04 0000 1
rs485raw 50 03 0000 1
rs485raw-repeat 20 50 04 0000 1
```

**Success only if:** `CRC RESULT: PASS` and frame like `32 84 02 32 CE` (or a normal CRC-valid response).
