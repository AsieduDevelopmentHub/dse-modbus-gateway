"""Read each GenComm page from the connected DSE and save what came back.

Writes doc/dse_register_map/live_7320.json after every page so a disconnect
keeps the registers already captured. Read-only: FC03 only, no SCF writes.
"""

import json
import re
import time
from pathlib import Path

import serial

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "doc" / "dse_register_map" / "gencomm_pages.json"
OUT = ROOT / "doc" / "dse_register_map" / "live_7320.json"
PORT = "COM8"

REG_RE = re.compile(r"Register\[(\d+)\]=0x([0-9A-Fa-f]+)")
EXC_RE = re.compile(r"Exception 0x([0-9A-Fa-f]+)")


def is_sentinel(word):
    return (0xFFF8 <= word <= 0xFFFF) or (0x7FF8 <= word <= 0x7FFF)


def load_catalog():
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    names = {}
    for page in data["pages"]:
        for reg in page["registers"]:
            for off in range(reg["offset"], reg["offsetEnd"] + 1):
                names.setdefault((page["page"], off), reg["name"])
    return data, names


def open_port():
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = 115200
    ser.timeout = 0
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.dtr = False
    ser.rts = False
    time.sleep(2.2)
    ser.reset_input_buffer()
    return ser


def send(ser, cmd, timeout=4.5):
    ser.write((cmd + "\r\n").encode("ascii"))
    buf = ""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(8192)
        if chunk:
            buf += chunk.decode("utf-8", "replace")
            if "RESULT:" in buf:
                time.sleep(0.04)
                extra = ser.read(8192)
                if extra:
                    buf += extra.decode("utf-8", "replace")
                break
        else:
            time.sleep(0.02)
    return buf


def parse_block(text, address, qty):
    words = {}
    for match in REG_RE.finditer(text):
        words[int(match.group(1))] = int(match.group(2), 16)
    ordered = [words[i] for i in range(qty) if i in words]
    if "VALID MODBUS RESPONSE" in text and len(ordered) == qty:
        return "ok", ordered, None
    if "VALID MODBUS EXCEPTION" in text or "EXCEPTION" in text:
        exc = EXC_RE.search(text)
        code = int(exc.group(1), 16) if exc else None
        return "exception", [], code
    if "TIMEOUT" in text:
        return "timeout", ordered, None
    if "CRC" in text:
        return "crc", ordered, None
    return "unknown", ordered, None


def read_qty(ser, address, qty):
    last = ("unknown", [], None)
    for _ in range(2):
        text = send(ser, f"read {address} {qty}")
        status, words, code = parse_block(text, address, qty)
        last = (status, words, code)
        if status in ("ok", "exception"):
            return last
    return last


def read_span(ser, base, last_offset):
    """Walk the page. Prefer 16-word reads, then 8, then 2."""
    words = {}
    errors = []
    offset = 0
    end = min(last_offset, 255)
    preferred = 16
    while offset <= end:
        addr = base + offset
        remaining = end - offset + 1
        qty = min(preferred, remaining)
        status, got, code = read_qty(ser, addr, qty)
        if status == "exception" and qty > 8:
            preferred = 8
            continue
        if status == "exception" and qty > 2:
            preferred = 2
            continue
        if status == "exception":
            errors.append({"address": addr, "exception": code})
            break
        if status != "ok":
            errors.append({"address": addr, "status": status})
            offset += qty
            continue
        for i, raw in enumerate(got):
            words[offset + i] = raw
        offset += qty
    return words, errors


def main():
    catalog, names = load_catalog()
    ser = open_port()
    pages_out = []
    snapshot = {
        "device": "DSE 7320 / 7320 MKII",
        "slave": 1,
        "baud": "9600 8N1",
        "function": "FC03 holding registers",
        "note": "Live read while connected. Missing pages returned a Modbus exception and were stopped there. 0xFFF8-0xFFFF and 0x7FF8-0x7FFF are GenComm sentinels.",
        "pages": pages_out,
    }
    try:
        for page in catalog["pages"]:
            last = max(r["offsetEnd"] for r in page["registers"])
            print(f"page {page['page']} base {page['base']} through +{last}", flush=True)
            words, errors = read_span(ser, page["base"], last)
            values = []
            sentinel = 0
            for off in sorted(words):
                raw = words[off]
                if is_sentinel(raw):
                    sentinel += 1
                    continue
                values.append(
                    {
                        "offset": off,
                        "address": page["base"] + off,
                        "raw": raw,
                        "name": names.get((page["page"], off), ""),
                    }
                )
            entry = {
                "page": page["page"],
                "base": page["base"],
                "title": page["title"],
                "wordsRead": len(words),
                "sentinelWords": sentinel,
                "values": values,
                "errors": errors,
            }
            pages_out.append(entry)
            snapshot["pageCount"] = len(pages_out)
            snapshot["valueCount"] = sum(len(p["values"]) for p in pages_out)
            OUT.write_text(json.dumps(snapshot, indent=2), encoding="utf-8")
            print(
                f"  words {len(words)} values {len(values)} errors {len(errors)}",
                flush=True,
            )
    finally:
        ser.close()
    print(f"wrote {OUT}", flush=True)


if __name__ == "__main__":
    main()
