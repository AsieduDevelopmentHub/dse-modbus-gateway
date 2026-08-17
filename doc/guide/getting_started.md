# Getting started — DSE 7320 Modbus RTU Gateway

ESP32-C3 SuperMini + MAX485 as a **multi-device Modbus RTU master**.

Primary target: Deep Sea Electronics 7320.  
Phase 2 physical test device: Invendis ET1641 DC Energy Meter (slave 50).

Manual USB Serial CLI only. No MQTT, Wi-Fi, or UI.

---

## Wiring (ESP32-C3 → MAX485)

| MAX485 | ESP32-C3              |
| ------ | --------------------- |
| VCC    | **5V / VBUS**         |
| GND    | GND                   |
| DI     | GPIO21 (TX)           |
| RO     | GPIO20 (RX)           |
| DE+RE  | GPIO4 (tied together) |

Direction: GPIO4 HIGH = transmit, LOW = receive.

**Hardware note:** This MAX485 module is a 5V device. RO is a 5V-domain output — ESP32-C3 GPIO level compatibility must be treated as a hardware consideration and is **not** claimed automatically safe.

MAX485 A/B → device RS485 A/B (swap A/B if timeout with otherwise correct settings).

---

## Build / flash / monitor

```bash
pio run -t upload --upload-port COM6
pio device monitor --port COM6
```

ESP32-C3 SuperMini uses **native USB CDC**. After upload, open the monitor (or press RESET) so the banner appears. `monitor_rts/dtr = 0` and `monitor_echo = yes` are set in `platformio.ini`.

Close PuTTY / other serial apps before upload.

---

## CLI overview

```text
help
status
config
devices
device list
device select <name|index>
device show
read <address> <quantity>
readhex <hex_address> <quantity>
rawread <slave> <fc> <hex_addr> <quantity>
set slave <id>
set fc <03|04>
```

Device keys: `dse7320` (default), `et1641`.

---

## Phase 2 — ET1641 physical test procedure

1. Power ET1641; confirm Modbus = slave **50**, **9600 8N1**.
2. Wire MAX485 A/B to ET1641 RS485 A/B (common GND with ESP32).
3. Flash/monitor the ESP32.
4. In the CLI:

```text
devices
device select et1641
device show
rawread 50 03 0000 1
```

5. Interpret result:
   - **Normal response + CRC OK** → link proven.
   - **Modbus exception + CRC OK** (e.g. Illegal Data Address) → link proven; register/FC may be invalid (expected — do not guess maps).
   - **Timeout** → check power, A/B polarity, slave ID, baud, DE/RE, MAX485 wiring, cable.

Do **not** treat address `0000` as a documented ET1641 register. It is only a bus probe.

---

## DSE test (when controller is available)

```text
device select dse7320
readhex 0405 1
```

Confirm `0x0405` in official DSE docs before trusting any engineering meaning. Raw hex/decimal only until a register map is added.

Named DSE readings (GenComm + S6 map):

```text
device select dse7320
regs
get battery_v
get eng_rpm
get eng_status
get oil_press
poll engine
poll alarms
alarms
poll scf
```

---

## Config

- Pins / DSE compile-time defaults: `src/config/config.h`
- Device profiles: `src/devices/modbus_device.cpp`
