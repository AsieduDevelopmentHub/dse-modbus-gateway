# DSE 7320 Modbus RTU — Current Phase Status

**Date:** 2026-08-10  
**Project:** `dse7320_rtu`  
**Phase:** PHASE 2A — IN PROGRESS (RS485 / Modbus diagnostics)

---

## Verdict

| Phase | Status |
|-------|--------|
| Phase 1 — USB CDC + Modbus CLI | COMPLETE |
| Phase 2 — Live CRC-valid ET1641 frame | **NOT COMPLETE** |
| Phase 2A — Firmware/protocol diagnostics | **IN PROGRESS** |

Do **not** declare physical Modbus success until a CRC-valid ET1641 frame is captured.

Observed so far: TX frames are correct; some RX buffers show `32 84 02 ?? ??` with **invalid CRC** (often after A/B experiments). Invalid-CRC frames must **not** be treated as Modbus exceptions.

---

## Hardware (lab)

- ESP32-C3 SuperMini, UART1: RX20 / TX21 / DE·RE GPIO4  
- MAX485 powered from **ESP32-C3 3.3V** (document accurately; no hardware redesign in 2A)  
- USB CDC (CLI) and UART1 (RS485) are separate  

---

## Phase 2A diagnostic commands

```text
crc-test
diagnostic on|off
rawread 50 04 0000 1
rawread 50 03 0000 1
repeat 20 50 04 0000 1
config
```

CRC self-test vectors (offline):

- `32 03 00 00 00 01 81 C9`
- `32 04 00 00 00 01 34 09`
- `32 84 02 32 CE`

---

## Architecture

Multi-device master remains: DSE7320 (ID10) + ET1641 (ID50). DSE is not replaced.
