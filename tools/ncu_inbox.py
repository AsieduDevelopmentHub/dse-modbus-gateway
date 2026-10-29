"""Receive NCU1 batches and decrypt them with AES-128-GCM.

Wire layout, little-endian:
  0..3   ASCII NCU1
  4      version 1
  5      sample count
  6..11  MAC
  12..15 sequence
  16..27 nonce
  28..29 ciphertext length
  30     ciphertext
  end-16 GCM tag

The 30-byte header is the additional authenticated data.
"""

import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from cryptography.hazmat.primitives.ciphers.aead import AESGCM


def load_key(text):
    raw = bytes.fromhex(text.strip())
    if len(raw) != 16:
        raise SystemExit("key must be 32 hex characters (16 bytes)")
    return raw


def open_packet(blob, key):
    if len(blob) < 46 or blob[:4] != b"NCU1":
        raise ValueError("not an NCU1 packet")
    if blob[4] != 1:
        raise ValueError(f"unsupported version {blob[4]}")
    seq = int.from_bytes(blob[12:16], "little")
    nonce = blob[16:28]
    length = int.from_bytes(blob[28:30], "little")
    if 30 + length + 16 != len(blob):
        raise ValueError("length does not match packet")
    body = blob[30 : 30 + length]
    tag = blob[30 + length :]
    plain = AESGCM(key).decrypt(nonce, body + tag, blob[:30])
    return seq, json.loads(plain.decode("utf-8"))


class Handler(BaseHTTPRequestHandler):
    key = b""

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        blob = self.rfile.read(length)
        try:
            seq, payload = open_packet(blob, self.key)
        except Exception as exc:
            print(f"reject {exc}")
            self.send_response(400)
            self.end_headers()
            return
        print(f"seq {seq} samples {payload.get('n')} boot {payload.get('boot')}")
        print(json.dumps(payload, indent=2))
        self.send_response(204)
        self.end_headers()

    def log_message(self, fmt, *args):
        return


def main():
    parser = argparse.ArgumentParser(description="Decrypt NCU1 uplink batches")
    parser.add_argument("--key", required=True, help="32 hex characters")
    parser.add_argument("--listen", default="0.0.0.0:8080")
    args = parser.parse_args()
    key = load_key(args.key)
    host, _, port = args.listen.rpartition(":")
    Handler.key = key
    server = ThreadingHTTPServer((host or "0.0.0.0", int(port)), Handler)
    print(f"listening on {host or '0.0.0.0'}:{port}")
    server.serve_forever()


if __name__ == "__main__":
    main()
