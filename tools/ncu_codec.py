"""NCU1 framing from src/uplink/uplink.cpp.

Wire layout, little-endian:
  0..3   ASCII NCU1
  4      version 1
  5      sample count
  6..11  efuse MAC, the device identity
  12..15 sequence, little-endian
  16..27 nonce
  28..29 plaintext length
  30     ciphertext
  end-16 GCM tag

The 30-byte header is the AES-GCM additional data. The nonce is not repeated
inside the ciphertext. Sequence increments only after the device receives a
2xx response, so a failed POST is retried with the same sequence.
"""

import json

from cryptography.hazmat.primitives.ciphers.aead import AESGCM


def load_key(text):
    raw = bytes.fromhex(text.strip())
    if len(raw) != 16:
        raise ValueError("key must be 32 hex characters (16 bytes)")
    return raw


def identity_hex(mac):
    if len(mac) != 6:
        raise ValueError("MAC must be 6 bytes")
    return mac.hex()


def seal(plain, key, mac, seq, nonce, count=None):
    """Build a packet the same way Uplink::seal does. Used by tests."""
    if isinstance(plain, str):
        plain = plain.encode("utf-8")
    header = bytearray(30)
    header[0:4] = b"NCU1"
    header[4] = 1
    payload = json.loads(plain)
    header[5] = int(payload["n"] if count is None else count)
    header[6:12] = mac
    header[12:16] = int(seq).to_bytes(4, "little")
    header[16:28] = nonce
    header[28:30] = len(plain).to_bytes(2, "little")
    sealed = AESGCM(key).encrypt(nonce, plain, bytes(header))
    return bytes(header) + sealed


def open_packet(blob, key):
    if len(blob) < 46 or blob[:4] != b"NCU1":
        raise ValueError("not an NCU1 packet")
    if blob[4] != 1:
        raise ValueError(f"unsupported version {blob[4]}")
    count = blob[5]
    identity = blob[6:12].hex()
    seq = int.from_bytes(blob[12:16], "little")
    nonce = blob[16:28]
    length = int.from_bytes(blob[28:30], "little")
    if 30 + length + 16 != len(blob):
        raise ValueError("length does not match packet")
    body = blob[30 : 30 + length]
    tag = blob[30 + length :]
    plain = AESGCM(key).decrypt(nonce, body + tag, blob[:30])
    payload = json.loads(plain.decode("utf-8"))
    if payload.get("seq") != seq or payload.get("n") != count:
        raise ValueError("header does not match plaintext")
    samples = payload.get("s")
    if not isinstance(samples, list) or len(samples) != count or count < 1 or count > 8:
        raise ValueError("sample count is not valid")
    return {
        "seq": seq,
        "count": count,
        "identity": identity,
        "boot": payload.get("boot"),
        "samples": samples,
    }


def device_http_status(outcome):
    """Map a Convex result onto the status the ESP32 is waiting for.

    2xx clears the device batch. Unknown devices stay 403 so the batch is kept.
    """
    if outcome in {"accepted", "duplicate", "stale"}:
        return 204
    if outcome in {"unknown", "ambiguous", "no-generator"}:
        return 403
    return 502
