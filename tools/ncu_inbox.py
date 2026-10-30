"""Receive NCU1 batches, decrypt them, and hand them to Convex.

The ESP32 posts the sealed packet to this process. This process never creates
organizations or generators. It sends the efuse MAC from the packet header to
POST {INGEST_URL}. Convex resolves that identity to a device that was already
registered. Set the device identity to the 12 lowercase hex characters of the
MAC before enabling uplink.

Environment, never printed:
  INGEST_SECRET   bearer shared with the Convex INGEST_SECRET
  INGEST_URL      https://<deployment>.convex.site/ncu/ingest
  NCU_KEY         optional 32 hex characters instead of --key

--print-only decrypts, acknowledges, and drops the batch. Convex is not
updated. It prints that warning on startup and is not for production.
The default refuses to acknowledge a batch it did not store.
"""

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from ncu_codec import device_http_status, load_key, open_packet


def log_event(event, **fields):
    safe = {
        key: value
        for key, value in fields.items()
        if key not in {"key", "secret", "authorization", "samples"}
    }
    print(json.dumps({"event": event, **safe}, sort_keys=True), flush=True)


def post_convex(url, secret, body):
    data = json.dumps(body).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=data,
        method="POST",
        headers={
            "Authorization": f"Bearer {secret}",
            "Content-Type": "application/json",
            "Content-Length": str(len(data)),
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=2) as response:
            payload = json.loads(response.read().decode("utf-8") or "{}")
            return response.status, payload.get("status", "failed")
    except urllib.error.HTTPError as exc:
        raw = exc.read().decode("utf-8", errors="replace")
        try:
            payload = json.loads(raw or "{}")
        except json.JSONDecodeError:
            payload = {}
        return exc.code, payload.get("status", "failed")


class Handler(BaseHTTPRequestHandler):
    key = b""
    ingest_url = ""
    ingest_secret = ""
    print_only = False

    def do_POST(self):
        started = time.perf_counter()
        length = int(self.headers.get("Content-Length", "0"))
        if length < 46 or length > 900:
            self._finish(400, "malformed", started, identity="", count=0, accepted=0, rejected=1)
            return
        blob = self.rfile.read(length)
        try:
            opened = open_packet(blob, self.key)
        except Exception as exc:
            self._finish(400, "rejected", started, identity="", count=0, accepted=0, rejected=1, reason=type(exc).__name__)
            return
        if self.print_only:
            self._finish(
                204,
                "print-only",
                started,
                identity=opened["identity"],
                count=opened["count"],
                accepted=0,
                rejected=0,
                seq=opened["seq"],
            )
            return
        if not self.ingest_url or not self.ingest_secret:
            self._finish(503, "unconfigured", started, identity=opened["identity"], count=opened["count"], accepted=0, rejected=opened["count"])
            return
        try:
            http_status, outcome = post_convex(
                self.ingest_url,
                self.ingest_secret,
                {
                    "identity": opened["identity"],
                    "boot": opened["boot"],
                    "seq": opened["seq"],
                    "samples": opened["samples"],
                },
            )
        except Exception as exc:
            self._finish(
                502,
                "convex-failed",
                started,
                identity=opened["identity"],
                count=opened["count"],
                accepted=0,
                rejected=opened["count"],
                reason=type(exc).__name__,
            )
            return
        if http_status >= 500 or outcome == "failed":
            device_status = 502
            accepted = 0
            rejected = opened["count"]
        else:
            device_status = device_http_status(outcome)
            accepted = opened["count"] if outcome == "accepted" else 0
            rejected = 0 if outcome in {"accepted", "duplicate", "stale"} else opened["count"]
        self._finish(
            device_status,
            outcome,
            started,
            identity=opened["identity"],
            count=opened["count"],
            accepted=accepted,
            rejected=rejected,
            seq=opened["seq"],
        )

    def _finish(self, status, outcome, started, **fields):
        log_event(
            outcome,
            status=status,
            ms=round((time.perf_counter() - started) * 1000, 1),
            **fields,
        )
        self.send_response(status)
        self.end_headers()

    def log_message(self, fmt, *args):
        return


def load_local_env():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".env")
    if not os.path.isfile(path):
        return
    with open(path, encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip().strip('"').strip("'")
            if key and key not in os.environ:
                os.environ[key] = value


def warn_print_only():
    sys.stderr.write(
        "print-only is not for production.\n"
        "Packets are decrypted.\n"
        "Packets are acknowledged.\n"
        "Telemetry is dropped.\n"
        "Convex is not updated.\n"
    )
    sys.stderr.flush()


def main():
    load_local_env()
    parser = argparse.ArgumentParser(description="Decrypt NCU1 uplink batches and store them")
    parser.add_argument("--key", default=os.environ.get("NCU_KEY", ""), help="32 hex characters, or set NCU_KEY")
    parser.add_argument("--listen", default="0.0.0.0:8080")
    parser.add_argument("--ingest-url", default=os.environ.get("INGEST_URL", ""))
    parser.add_argument(
        "--print-only",
        action="store_true",
        help="Decrypt, acknowledge, and drop the batch. Convex is not updated. Not for production.",
    )
    args = parser.parse_args()
    if args.print_only:
        warn_print_only()
    if not args.key:
        raise SystemExit("pass --key or set NCU_KEY")
    if not args.print_only and not args.ingest_url:
        raise SystemExit("pass --ingest-url or --print-only")
    secret = os.environ.get("INGEST_SECRET", "")
    if not args.print_only and not secret:
        raise SystemExit("set INGEST_SECRET")
    key = load_key(args.key)
    host, _, port = args.listen.rpartition(":")
    Handler.key = key
    Handler.ingest_url = args.ingest_url
    Handler.ingest_secret = secret
    Handler.print_only = args.print_only
    server = ThreadingHTTPServer((host or "0.0.0.0", int(port)), Handler)
    log_event("listening", host=host or "0.0.0.0", port=int(port), ingest=bool(args.ingest_url and not args.print_only))
    server.serve_forever()


if __name__ == "__main__":
    main()
