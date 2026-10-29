"""Turn the GenComm page tables into doc/dse_register_map/gencomm_pages.json.

Reads a text extract of the register tables (pipe rows). Address = page * 256
+ offset. Continuation rows with a blank offset are bit fields of the
previous register.
"""

import json
import re
import sys
from pathlib import Path

OFFSET_RE = re.compile(r"^(\d+)(?:\s*-\s*(\d+))?$")


def cells(line):
    parts = [c.strip() for c in line.strip().strip("|").split("|")]
    return parts


def parse(text, scf_keys):
    start = text.find("Page 0", 40000)
    if start < 0:
        start = text.find("Page 0")
    body = text[start:]
    marks = list(
        re.finditer(r"(?:^|\n)(?:\d+(?:\.\d+)*\s+)?Page (\d+)\b", body)
    )
    pages = []
    for i, mark in enumerate(marks):
        page = int(mark.group(1))
        end = marks[i + 1].start() if i + 1 < len(marks) else len(body)
        chunk = body[mark.start() : end]
        title = chunk.split("\n", 1)[0].strip()
        regs = []
        last = None
        for line in chunk.splitlines():
            if not line.startswith("|"):
                continue
            col = cells(line)
            if len(col) < 2:
                continue
            head = col[0]
            if head.lower() in ("register offset", "page register offset", "---"):
                continue
            if set(head) <= set("-: "):
                continue
            name = col[1]
            if not name or name.lower() in ("name", "---"):
                continue
            key_cell = next(
                (c.replace(" ", "") for c in col if re.fullmatch(r"357\d\d", c.replace(" ", ""))),
                None,
            )
            if key_cell:
                scf_keys.append(
                    {
                        "function": head,
                        "name": name,
                        "key": int(key_cell),
                    }
                )
                continue
            m = OFFSET_RE.match(head)
            if m:
                off = int(m.group(1))
                off_end = int(m.group(2)) if m.group(2) else off
                if off_end < off or off_end - off > 15:
                    continue
                # Numbered notes (password levels, exception codes) reuse an
                # offset that the table already defined. Keep the table row.
                if any(r["offset"] == off for r in regs):
                    continue
                width = off_end - off + 1
                row = {
                    "offset": off,
                    "offsetEnd": off_end,
                    "address": page * 256 + off,
                    "width": width,
                    "name": name,
                    "columns": col[2:],
                }
                regs.append(row)
                last = row
            elif head == "" and last is not None and name.lower() not in ("name",):
                last.setdefault("fields", []).append(
                    {"name": name, "columns": col[2:]}
                )
        if not regs:
            continue
        if any(p["page"] == page for p in pages):
            continue
        pages.append(
            {
                "page": page,
                "base": page * 256,
                "title": title,
                "registers": regs,
            }
        )
    return pages


def main():
    src = Path(sys.argv[1])
    dest = Path(sys.argv[2])
    scf_keys = []
    pages = parse(src.read_text(encoding="utf-8", errors="replace"), scf_keys)
    nreg = sum(len(p["registers"]) for p in pages)
    out = {
        "addressFormula": "page * 256 + offset",
        "wordOrder": "32-bit values are high word first, even aligned",
        "scale": "engineering = raw * scale factor",
        "sentinels": {
            "unsigned16": {
                "FFFF": "unimplemented",
                "FFFE": "over range",
                "FFFD": "under range",
                "FFFC": "transducer fault",
                "FFFB": "bad data",
            },
            "signed16": {
                "7FFF": "unimplemented",
                "7FFE": "over range",
                "7FFD": "under range",
                "7FFC": "transducer fault",
                "7FFB": "bad data",
            },
        },
        "scfKeys": scf_keys,
        "scfWrite": "FC16 to address 4104 (key) and 4105 (ones complement, 65535-key) in one request. Password level 2 or 3 is required. Page 0 register 6 is the password status.",
        "pageCount": len(pages),
        "registerCount": nreg,
        "pages": pages,
    }
    dest.write_text(json.dumps(out, indent=2), encoding="utf-8")
    print(f"pages {len(pages)} registers {nreg} -> {dest}")


if __name__ == "__main__":
    main()
