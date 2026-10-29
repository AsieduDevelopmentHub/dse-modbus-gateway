# dse7320_rtu

ESP32-C3 SuperMini + MAX485 Modbus RTU master for **Deep Sea Electronics (DSE) genset controllers**.

Primary family: **DSE 7320**, **DSE 7320 MKII**, and other 73xx-class DSE variants that share the same holding-register layout.

Also: Invendis ET1641 and future RS485 slaves.

**5V isolation benches:** Arduino Mega 2560 (full DSE named map via PROGMEM) / Uno / Nano (diagnostics-only `GATEWAY_LITE`) envs in `platformio.ini` — separate ESP32 3.3V RO-level issues from firmware bugs.

**Web UI (ESP32):** soft-AP + `/api/status` + embedded overview page. The React app is a separate repo at `dev/iot/dse-gateway-web`. See [doc/guide/web_ui.md](doc/guide/web_ui.md).

## Docs

| Path | Contents |
|------|----------|
| [doc/spec/mvp.md](doc/spec/mvp.md) | Original MVP requirements |
| [doc/spec/phase2.md](doc/spec/phase2.md) | Phase 2 multi-device / ET1641 task |
| [doc/status/phase_status.md](doc/status/phase_status.md) | Current phase status |
| [doc/guide/getting_started.md](doc/guide/getting_started.md) | Wiring (ESP32 + Arduino), build, test procedures |
| [doc/guide/cli.md](doc/guide/cli.md) | Full USB Serial CLI reference |
| [doc/guide/web_ui.md](doc/guide/web_ui.md) | Wi-Fi AP, JSON API, React app |

## Maintainer

Maintained by [Asiedu Minta Kwaku](https://asiedudevhub.auralenx.com/), a software developer and IoT & embedded systems engineer based in Ghana.

For related work and project enquiries, visit my [portfolio](https://asiedudevhub.auralenx.com/) or connect on [LinkedIn](https://www.linkedin.com/in/asiedudevelopmenthub).
