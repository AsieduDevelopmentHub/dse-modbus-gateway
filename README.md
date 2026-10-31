# iCanOva Edge Gateway

Embedded industrial edge gateway for connecting and monitoring industrial equipment through supported communication interfaces.

**Brand:** iCanOva — Imagine. Connect. Innovate.  
**Descriptor:** Intelligent technology for connected industries.  
A technology platform by ASIEDU DEVELOPMENT HUB (ADH).

This repository is the edge firmware and device software. The web monitoring platform lives in the sibling folder `dev/iot/icanova-monitoring-web` (GitHub name still `dse-gateway-web` until renamed).

## Purpose

The gateway talks to industrial controllers over RS485 (Modbus RTU), exposes a USB console for commissioning, and on ESP32-C3 serves a local Wi-Fi overview plus JSON APIs. Optional encrypted batch uplink can forward telemetry to a station host.

**Implemented today**

- Modbus RTU master for Deep Sea Electronics (DSE) 73xx-class genset controllers (named map + GenComm pages).
- Additional RS485 profiles (Invendis ET1641, EN-Volt) for lab mapping.
- USB Serial CLI with login, config filesystem, SCF control writes, and GenComm page capture tools.
- ESP32 soft-AP, `/api/status`, `/api/catalog`, `/api/read`, `/api/write`, `/api/control`.
- Optional NCU1 AES-GCM batched uplink (`doc/guide/uplink.md`).

**Not claimed as finished product features**

- Cellular modem productization (architecture may exist in docs; do not treat as shipped).
- Cloud multi-tenant operations (those live in the web platform).
- Final enclosure / logo artwork (logo on hold; see `doc/brand/`).

## Hardware

- **Production target:** ESP32-C3 SuperMini — UART1 RX20 / TX21 / DE·RE GPIO4, THVD1450 or level-safe RS485.
- **5V isolation benches:** Arduino Mega 2560 (full DSE map) / Uno / Nano (`GATEWAY_LITE` diagnostics).

Deep Sea Electronics remains a supported **equipment** manufacturer. DSE model and register documentation stay under `doc/dse_register_map/`.

## Build and flash

```bash
pio run -e esp32-c3-supermini
pio run -e esp32-c3-supermini -t upload --upload-port COMx
pio device monitor --port COMx
```

RTS/DTR off for native USB CDC. See [doc/guide/getting_started.md](doc/guide/getting_started.md).

## Configuration

Saved settings: `/etc/gateway.conf` via the USB console (`config set …`). Soft-AP SSID prefix remains `DSE-GW` for field compatibility. Default hostname for new installs is `icanova-gw`.

## Docs

| Path | Contents |
|------|----------|
| [doc/brand/](doc/brand/) | iCanOva brand specification |
| [doc/guide/getting_started.md](doc/guide/getting_started.md) | Wiring, build, test |
| [doc/guide/cli.md](doc/guide/cli.md) | USB Serial CLI |
| [doc/guide/web_ui.md](doc/guide/web_ui.md) | Wi-Fi API and embedded UI |
| [doc/guide/uplink.md](doc/guide/uplink.md) | Encrypted batched uplink |
| [doc/dse_register_map/](doc/dse_register_map/) | DSE / GenComm register maps |
| [doc/status/phase_status.md](doc/status/phase_status.md) | Phase status |

## Repository structure

```text
src/           firmware (CLI, Modbus, telemetry, HTTP, uplink)
doc/           guides, brand, register maps, specs
tools/         GenComm catalog, uplink inbox, capture helpers
platformio.ini multi-board environments
```

## Maintainer

Maintained by [Asiedu Minta Kwaku](https://asiedudevhub.auralenx.com/) / ASIEDU DEVELOPMENT HUB.

GitHub remote (pending authorized rename to `icanova-edge-gateway`): `AsieduDevelopmentHub/dse-modbus-gateway`.
