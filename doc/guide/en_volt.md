# Invendis EN-Volt — measured register map

Bench notes from 2026-10-28. Slave **63**, **9600 8N1**, function **03** only.
The ESP32-C3 gateway was on UART1 (RX 20, TX 21, DE/RE 4) with a THVD1450 at 3.3 V.
Every frame below had a CRC that checks over the **whole** buffer.

This is not a vendor map. Words are numbered from the first data byte. No units are assigned.
Named DSE reads stay on profile `dse7320`.

`envolt [slave]` (default 63) repeats this scan. It turns the Wi-Fi radio off; reset the board to bring the soft-AP back.

```text
rs485raw 63 03 0000 1
rs485raw 63 03 0002 1
rs485raw 63 03 001E 1
```

`rs485raw` prints a CRC check of the first 5 bytes, then a second check of the full buffer.
On the three blocks below, the 5-byte check fails and the full-buffer check passes.
`read` / `readhex` reject these frames because the byte-count field does not match the body.

---

## What the meter actually does

| Request | Result |
|---------|--------|
| FC04, any address tried | Exception 01, illegal function. Example: `3F 84 01 E3 0C` |
| FC03 address 0, 2, or 30 | Fixed image. Length does not follow quantity. CRC covers the whole frame |
| FC03 any other address 1–99 | Exception 02, illegal data address. Example: `3F 83 02 A1 3D` |
| FC03 even address 100–288, even quantity | Normal frame. Byte count is `quantity × 2`. Payload observed was all zeros |
| FC03 even address 100–288, quantity 1 | Exception 03, illegal data value. Example: `3F 83 03 60 FD` |
| FC03 odd address, or a read that **starts** at 290 or above (also 1000 and 2000) | Exception 02 |

A read may **start** at 288 and extend past it. `FC03 288 qty 4` returned four zero registers (288–291) in a normal 13-byte frame. `FC03 290 qty 10` was exception 02.

On addresses 0, 2, and 30 the third byte echoes `quantity × 2` (so qty 1 → `02`, qty 2 → `04`, qty 8 → `10`, qty 16 → `20`). The words after that byte stay the same image. Quantity 1, 2, 8, and 16 were all tried.

Leave about 40 ms between requests. Back-to-back polls drop the next reply and look like a silent address.

---

## Address 0 — 16 words, 37-byte frame

One live word. Word 11 climbs (15749, then 15769 on the next read). The others held still for the whole session.

| Word | Value | Notes |
|-----:|------:|-------|
| 0, 2, 4, 6, 7, 10, 12–15 | 0 | |
| 1 | 400 | Held |
| 3 | 114 | Held |
| 5 | 400 | Held |
| 8 | 2 | Held |
| 9 | 13722 | Held (`0x359A`) |
| 11 | 15769 | Climbs |

As big-endian 32-bit pairs (high word first, and that high word is 0): 400, 114, 400, 0, 144794 (`0x0002359A`), the climbing counter, 0, 0.

Example, CRC `C2 EC`:

```text
3F 03 02 00 00 01 90 00 00 00 72 00 00 01 90 00 00 00 00 00 02 35 9A 00 00 3D 85 00 00 00 00 00 00 00 00 C2 EC
```

---

## Address 2 — 10 words, 25-byte frame

Words 1, 3, and 5 change on every read. Word 7 moved from 40 to 42 over the session, then held. Word 9 held at 600.

| Word | Value | Notes |
|-----:|------:|-------|
| 0, 2, 4, 6, 8 | 0 | |
| 1 | 632 | Was 656 on the previous read |
| 3 | 25000 | Was 25032 |
| 5 | 1016 | Was 1020 |
| 7 | 42 | Slow |
| 9 | 600 | Held (`0x0258`) |

As big-endian 32-bit pairs: 632, 25000, 1016, 42, 600. Those three live values are the low half of the pair; the high half stayed 0.

Example, CRC `47 83`:

```text
3F 03 02 00 00 02 90 00 00 61 C8 00 00 03 FC 00 00 00 2A 00 00 02 58 47 83
```

---

## Address 30 (`0x001E`) — 8 words, 21-byte frame

Steady across rereads and across quantities.

| Word | Hex | Decimal |
|-----:|----:|--------:|
| 0 | `0x0000` | 0 |
| 1 | `0x4200` | 16896 |
| 2 | `0x0000` | 0 |
| 3 | `0x0000` | 0 |
| 4 | `0x0000` | 0 |
| 5 | `0x41F0` | 16880 |
| 6 | `0x0000` | 0 |
| 7 | `0x41F0` | 16880 |

Taking each pair with the **second** word as the IEEE-754 high half gives exactly 32.0, 0.0, 30.0, 30.0. That is a bit-pattern match, not a named measurement.

Example, CRC `50 43`:

```text
3F 03 02 00 00 42 00 00 00 00 00 00 00 41 F0 00 00 41 F0 50 43
```

---

## Addresses 100–288

Even start, even quantity (2, 4, 6, 10, 16), function 03: a normal response. Blocks walked at starts 100, 110, … 280, plus 288 quantity 2 and 4, were entirely zero. A start at 290 or any later address that was tried (through 500, plus 1000 and 2000) is exception 02.

Quantity 1 at 100 and at 280 is exception 03. Odd starts (101 quantity 2) are exception 02.
