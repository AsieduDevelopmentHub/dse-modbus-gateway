# Modbus S6 Parameter Configuration

Source: Deep Sea register table (screenshot `deepsea_register_map.png`) used for
firmware decoding in `src/devices/dse7320_registers.*`.

| # | Slave Device Address | Register Number | Registers Amount | Data Type | Reverse Order | Function Code | Bit Position | Register Initial Value | Factor | Offset | SPN | SPN Name | PGN | S6 Address (SA) | PGN Sending Period |
|---:|---:|---:|---:|---|:---:|---|---:|---:|---:|---:|---:|---|---:|---:|---:|
| 1 | 10 | 1029 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 167 | Charging System Potential (Voltage) | 65086 | 134 | 1000 |
| 2 | 10 | 1032 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2445 | Generator Phase A Line-Neutral AC RMS Voltage | 65027 | 134 | 1000 |
| 3 | 10 | 1034 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2446 | Generator Phase B Line-Neutral AC RMS Voltage | 65024 | 134 | 1000 |
| 4 | 10 | 1036 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2447 | Generator Phase C Line-Neutral AC RMS Voltage | 65021 | 134 | 1000 |
| 5 | 10 | 1038 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2441 | Generator Phase AB Line-Line AC RMS Voltage | 65027 | 134 | 1000 |
| 6 | 10 | 1040 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2442 | Generator Phase BC Line-Line AC RMS Voltage | 65024 | 134 | 1000 |
| 7 | 10 | 1042 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2443 | Generator Phase CA Line-Line AC RMS Voltage | 65021 | 134 | 1000 |
| 8 | 10 | 1044 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2449 | Generator Phase A AC RMS Current | 65027 | 134 | 1000 |
| 9 | 10 | 1046 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2450 | Generator Phase B AC RMS Current | 65024 | 134 | 1000 |
| 10 | 10 | 1048 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2451 | Generator Phase C AC RMS Current | 65021 | 134 | 1000 |
| 11 | 10 | 1031 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2436 | Generator Average AC Frequency | 65030 | 134 | 1000 |
| 12 | 10 | 1544 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2460 | Generator Total Apparent Power | 65029 | 134 | 1000 |
| 13 | 10 | 1536 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2452 | Generator Total Real Power | 65029 | 134 | 1000 |
| 14 | 10 | 1552 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2456 | Generator Total Reactive Power | 65028 | 134 | 1000 |
| 15 | 10 | 1052 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2453 | Generator Phase A Real Power | 65026 | 134 | 1000 |
| 16 | 10 | 1054 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2454 | Generator Phase B Real Power | 65023 | 134 | 1000 |
| 17 | 10 | 1056 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2455 | Generator Phase C Real Power | 65020 | 134 | 1000 |
| 18 | 10 | 1030 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 589 | Alternator Speed | 65237 | 134 | 1000 |
| 19 | 10 | 1554 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.01 | 0 | 2465 | Generator Phase A Power Factor | 65025 | 134 | 1000 |
| 20 | 10 | 1555 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.01 | 0 | 2466 | Generator Phase B Power Factor | 65022 | 134 | 1000 |
| 21 | 10 | 1556 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.01 | 0 | 2467 | Generator Phase C Power Factor | 65019 | 134 | 1000 |
| 22 | 10 | 1557 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.01 | 0 | 2464 | Generator Overall Power Factor | 65028 | 134 | 1000 |
| 23 | 10 | 1666 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2448 | Generator Average AC RMS Current | 65030 | 134 | 1000 |
| 24 | 10 | 1546 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2457 | Generator Phase A Reactive Power | 65025 | 134 | 1000 |
| 25 | 10 | 1548 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2458 | Generator Phase B Reactive Power | 65022 | 134 | 1000 |
| 26 | 10 | 1550 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2459 | Generator Phase C Reactive Power | 65019 | 134 | 1000 |
| 27 | 10 | 1538 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2461 | Generator Phase A Apparent Power | 65025 | 134 | 1000 |
| 28 | 10 | 1540 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2462 | Generator Phase B Apparent Power | 65022 | 134 | 1000 |
| 29 | 10 | 1542 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 2463 | Generator Phase C Apparent Power | 65019 | 134 | 1000 |
| 30 | 10 | 1800 | 2 | Integer (4 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 2468 | Generator Total kW Hours Export | 65018 | 134 | 1000 |
| 31 | 10 | 1558 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 0.1 | 0 | 3590 | Generator Total Percent kW | 64911 | 134 | 1000 |
| 32 | 10 | 46648 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521839 | Stop LED | 63991 | 134 | 1000 |
| 33 | 10 | 46649 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521840 | Manual LED | 63991 | 134 | 1000 |
| 34 | 10 | 46650 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521841 | Test LED | 63991 | 134 | 1000 |
| 35 | 10 | 46651 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521842 | Auto LED | 63991 | 134 | 1000 |
| 36 | 10 | 46652 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521843 | Mains LED | 63991 | 134 | 1000 |
| 37 | 10 | 46653 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521844 | Mains Breaker LED | 63991 | 134 | 1000 |
| 38 | 10 | 46654 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521845 | Gen Breaker LED | 63991 | 134 | 1000 |
| 39 | 10 | 46655 | 1 | Short Integer (2 bytes) | No | Read Holding Registers (03) | 0 | — | 1 | 0 | 521846 | Gen LED | 63991 | 134 | 1000 |

## Firmware keys

| Key | Register | Unit |
|-----|---------:|------|
| `battery_v` | 1029 | V |
| `eng_rpm` | 1030 | rpm |
| `gen_van` / `gen_vbn` / `gen_vcn` | 1032 / 1034 / 1036 | V |
| `gen_vab` / `gen_vbc` / `gen_vca` | 1038 / 1040 / 1042 | V |
| `gen_ia` / `gen_ib` / `gen_ic` | 1044 / 1046 / 1048 | A |
| `gen_freq` | 1031 | Hz |
| `gen_p_tot` / `gen_q_tot` / `gen_s_tot` | 1536 / 1552 / 1544 | W / VAr / VA |
| `gen_pf` / `gen_pfa`… | 1557 / 1554… | — |
| `gen_kwh_exp` | 1800 | kWh |
| `gen_pct_kw` | 1558 | % |
| `led_stop` … `led_gen` | 46648 … 46655 | — |

### Also from GenComm (`register_clone_dse8610.md`)

| Key | Register | Notes |
|-----|---------:|-------|
| `mfr_code` / `model` / `serial` | 768 / 769 / 770 | Identity |
| `op_mode` / `eng_status` | 772 / 1408 | Enumerated state |
| `oil_press` / `coolant_temp` / `oil_temp` / `fuel_level` | 1024–1027 | Engine |
| `eng_hours` / `eng_starts` | 1798 / 1808 | Hours from seconds; start count |
| `scf_f0`…`scf_f7` | 4096–4103 | SCF support flags (read) |
| `scf_key` / `scf_key_inv` | 4104 / 4105 | Control write pair (use CLI `scf …`) |
| `charge_alt_v` | 1028 | V (charger alternator) |
| `mains_freq` / `mains_van`… | 1059 / 1060… | Utility/mains |
| `oil_level` / `coolant_level` | 1357 / 1359 | % |
| `chg_i_out` / `chg_v_out` | 1256 / 1258 | A / V (mA/mV raw) |
| `alm_39425`…`alm_39445` | 39425–39445 | Packed 4×4-bit alarms |
| `alm_di_0`…`alm_di_3` | 39169–39172 | Digital input alarms |
| `alm_plc_0`…`alm_plc_2` | 39476–39478 | PLC/user alarms |

### Variances vs `deepsea_clone1_dse7320.json`

| Topic | Clone JSON | Firmware (kept) |
|-------|------------|-----------------|
| Address form | Hex page/offset (`0405`) | Decimal GenComm (`1029`) |
| Frequency scale | Gauge uses ×0.01 (also ×1 in status) | ×0.1 (GenComm / Victron) |
| Run hours | Treated as hours | Seconds → hours (÷3600) |
| Total power display | ×0.001 → kW | Raw watts (`gen_p_tot`) |
| Phase power L1/L2/L3 | `041a/041c/041e` = 1050/1052/1054 | **1052/1054/1056** (GenComm/S6/Victron) |
| Alarms page 154 | 32-bit pairs (`9a01` len 4) | Per-register 16-bit nibble decode (same bits) |
| Utility L1 | 16-bit @1060 (JSON notes docs say 32-bit) | 16-bit @1060 as JSON; L2/L3 32-bit |
| SCF `resetmaint` | Key 35738 | `scf reset_maint` |

Skipped as low-value/unknown: `008b` UNK1.

### SCF writes (CLI)

```text
scf list
scf flags
scf auto
scf telemetry_start
scf telemetry_stop
scf test_on_load
scf raw 35703
```

`scf stop` (key 35700) is **not implemented** in this firmware.

Writes key + complement (65535−key) to 4104/4105 in one FC16 transaction.

## Notes

- **Slave Device Address:** `10` for all entries.
- **Function Code:** `Read Holding Registers (03)` for all entries.
- **Word order:** Reverse order = No → 32-bit = high register word first.
- **Scaling:** engineering value = `raw × factor` (offset 0).
- LED block corrected to **46648–46655** from the screenshot (not 48648).
- Overall power factor factor set to **0.01** to match phase PF scaling.
- CLI: `regs`, `get <key|#|addr>`, `poll […]`, `alarms`, `scf list|flags|<cmd>|raw <key>`.
