"""Build the compact GenComm catalog used by the ESP32 and the web repo.

Reads doc/dse_register_map/gencomm_pages.json and live_7320.json.
Writes src/net/gencomm_catalog.h, and public/gencomm.json in
dev/iot/dse-gateway-web when that folder exists.
"""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "doc" / "dse_register_map" / "gencomm_pages.json"
LIVE = ROOT / "doc" / "dse_register_map" / "live_7320.json"
WEB_REPO = ROOT.parent.parent / "iot" / "dse-gateway-web"
WEB_OUT = WEB_REPO / "public" / "gencomm.json"
HDR_OUT = ROOT / "src" / "net" / "gencomm_catalog.h"

TYPE_RE = re.compile(r"^(8|16|32|16S|32S|16/16|15/16-16/16)$", re.I)

# These shut the set down. The HTTP control API rejects them.
BLOCKED_KEYS = {35700, 35776}

# Mode, start, and breaker commands. The UI asks before sending them.
RISK_KEYS = {35701, 35702, 35703, 35704, 35705, 35708, 35709, 35738}


def access(page, name, cols, address):
    blob = " ".join(cols).lower()
    if 4104 <= address <= 4105:
        return "r"
    if "read/write" in blob:
        return "rw"
    if "write only" in blob:
        return "w"
    if "read only" in blob:
        return "r"
    if page >= 128 and "raw status" not in name.lower():
        return "rw"
    return "r"


def is_noise(name):
    n = name.strip().lower()
    return n in ("unimplemented", "reserved") or n.startswith("unimplemented")


def is_complement(name):
    n = name.lower()
    return "compliment" in n or "complement" in n


def data_type(cols):
    for col in cols:
        if TYPE_RE.match(col.strip()):
            return col.strip()
    return ""


def page_title(page):
    title = (page.get("title") or "").replace("\ufffd", " ").strip()
    title = re.sub(r"^Page\s+\d+\s*", "", title)
    title = re.sub(r"^[\W_]+", "", title).strip()
    return title


def main():
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    live = json.loads(LIVE.read_text(encoding="utf-8"))
    live_words = {}
    for page in live["pages"]:
        for value in page["values"]:
            live_words[value["address"]] = value["raw"]

    pages_out = []
    kept = 0
    writable = 0
    for page in catalog["pages"]:
        regs = []
        for reg in page["registers"]:
            name = reg["name"].strip()
            if is_noise(name):
                continue
            if is_complement(name) and regs:
                regs[-1]["comp"] = True
                continue
            cols = reg.get("columns") or []
            addr = reg["address"]
            acc = access(page["page"], name, cols, addr)
            row = {
                "o": reg["offset"],
                "w": reg["width"],
                "n": name,
                "a": acc,
            }
            kind = data_type(cols)
            if kind:
                row["ty"] = kind
            if cols and cols[0] != "":
                row["mn"] = cols[0]
            if len(cols) > 1 and cols[1] != "":
                row["mx"] = cols[1]
            words = []
            have = False
            for i in range(reg["width"]):
                if addr + i in live_words:
                    words.append(live_words[addr + i])
                    have = True
                else:
                    words.append(None)
            if have:
                row["v"] = words
            if acc in ("rw", "w"):
                writable += 1
            regs.append(row)
            kept += 1
        if not regs:
            continue
        title = page_title(page) or regs[0]["n"]
        pages_out.append(
            {
                "p": page["page"],
                "b": page["base"],
                "t": title,
                "regs": regs,
            }
        )

    scf = []
    seen_keys = set()
    for item in catalog["scfKeys"]:
        key = int(item["key"])
        name = item["name"].strip()
        if not name or name == str(key) or key in seen_keys:
            continue
        seen_keys.add(key)
        low = name.lower()
        blocked = key in BLOCKED_KEYS or "off mode" in low
        entry = {"k": key, "n": name}
        if blocked:
            entry["blocked"] = True
        elif key in RISK_KEYS or "start" in low or "transfer" in low:
            entry["risk"] = True
        scf.append(entry)

    out = {
        "device": "DSE 7320 / 7320 MKII",
        "model": 32841,
        "formula": "address = page * 256 + offset",
        "wordOrder": "32-bit values are high word first",
        "note": "v is the last FC03 snapshot from this module. a=rw or a=w can be written with FC16. Password rows with comp send the value and its ones complement together. Control keys are separate from register writes.",
        "pages": pages_out,
        "scf": scf,
    }
    text = json.dumps(out, ensure_ascii=False, separators=(",", ":"))
    if WEB_REPO.is_dir():
        WEB_OUT.parent.mkdir(parents=True, exist_ok=True)
        WEB_OUT.write_text(text, encoding="utf-8")
    else:
        print(f"skip web catalog; {WEB_REPO} is not present")
    if ")GENJSON" in text:
        raise SystemExit("catalog contains the C++ raw-string delimiter")
    header = (
        "#pragma once\n\n"
        "// Generated by tools/build_gencomm_catalog.py. Do not edit.\n"
        "static const char GENCOMM_CATALOG[] PROGMEM = R\"GENJSON(\n"
        + text
        + "\n)GENJSON\";\n"
    )
    HDR_OUT.write_text(header, encoding="utf-8")
    print(
        f"pages {len(pages_out)} registers {kept} writable {writable} "
        f"scf {len(scf)} bytes {len(text)}"
    )


if __name__ == "__main__":
    main()
