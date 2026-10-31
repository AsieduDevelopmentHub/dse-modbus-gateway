# DSE 7320 Modbus RTU — Current Phase Status

**Date:** 2026-10-20  
**Project:** `dse7320_rtu`  
**Phase:** PHASE 3A — IN PROGRESS (local web UI + JSON API)

---

## Verdict

| Phase | Status |
|-------|--------|
| Phase 1 — USB CDC + Modbus CLI | COMPLETE |
| Phase 2 — Live CRC-valid ET1641 / DSE frame | NOT COMPLETE (lab RO-level risk on ESP32) |
| Phase 2A — Firmware/protocol diagnostics | COMPLETE (CLI tools remain) |
| Phase 3A — ESP32 Wi-Fi API + overview UI | **IN PROGRESS** |

Do **not** declare physical Modbus success until a CRC-valid frame is captured on the live bus. The web UI shows last-good / fail state when Modbus refresh fails.

---

## Phase 3A deliverables

- Soft-AP `ICANOVA-EDGE-XXXX` + `GET /`, `/api/status`, `/api/health`
- Telemetry snapshot (mode, RPM, battery, V/A/kW, alarms)
- React app in `dev/iot/icanova-monitoring-web` (same JSON contract + mock)
- Guide: [doc/guide/web_ui.md](../guide/web_ui.md)

---

## Hardware (lab)

- ESP32-C3 SuperMini, UART1: RX20 / TX21 / DE·RE GPIO4  
- MAX485: prefer 5V VCC; **RO → ESP32 still needs level shift or 3.3V transceiver**  
- Arduino Mega 2560: 5V isolation bench with full DSE named map  

---

## Architecture

Multi-device master remains: DSE7320 + ET1641. USB CLI and HTTP run together on ESP32.
