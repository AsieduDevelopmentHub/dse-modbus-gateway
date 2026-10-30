import io
import json
import threading
import unittest
import urllib.error
import urllib.request
from contextlib import redirect_stdout
from http.server import ThreadingHTTPServer

import ncu_inbox
from ncu_codec import device_http_status, load_key, open_packet, seal


KEY = bytes.fromhex("00112233445566778899aabbccddeeff")
MAC = bytes.fromhex("aabbccddeeff")
NONCE = bytes.fromhex("000102030405060708090a0b")


def sample(rpm, kw100):
    return {
        "t": 1000,
        "age": 2,
        "link": "ok",
        "alm": 0,
        "mode": None,
        "rpm": rpm,
        "batt10": None,
        "hz10": None,
        "kw100": kw100,
        "van10": None,
    }


def plain(seq, samples):
    return json.dumps(
        {"boot": 7, "seq": seq, "n": len(samples), "s": samples},
        separators=(",", ":"),
    )


class NcuPacket(unittest.TestCase):
    def test_valid_packet_decrypts_and_keeps_nulls_and_negative_power(self):
        samples = [sample(None, -45), sample(1500, 0)]
        blob = seal(plain(4, samples), KEY, MAC, 4, NONCE)
        opened = open_packet(blob, KEY)
        self.assertEqual(opened["identity"], "aabbccddeeff")
        self.assertEqual(opened["seq"], 4)
        self.assertIsNone(opened["samples"][0]["rpm"])
        self.assertEqual(opened["samples"][0]["kw100"], -45)
        self.assertEqual(opened["samples"][1]["rpm"], 1500)

    def test_bad_tag_is_rejected(self):
        blob = bytearray(seal(plain(1, [sample(0, None)]), KEY, MAC, 1, NONCE))
        blob[-1] ^= 0x01
        with self.assertRaises(Exception):
            open_packet(bytes(blob), KEY)

    def test_malformed_packet_is_rejected(self):
        with self.assertRaises(ValueError):
            open_packet(b"NCU1" + b"\x00" * 10, KEY)

    def test_header_count_must_match_plaintext(self):
        blob = seal(plain(2, [sample(1, None)]), KEY, MAC, 2, NONCE, count=2)
        with self.assertRaises(ValueError):
            open_packet(blob, KEY)

    def test_retry_keeps_the_same_sequence(self):
        blob = seal(plain(3, [sample(10, -1)]), KEY, MAC, 3, NONCE)
        first = open_packet(blob, KEY)
        second = open_packet(blob, KEY)
        self.assertEqual(first["seq"], second["seq"])
        self.assertEqual(first["samples"], second["samples"])

    def test_device_status_keeps_unknown_batches(self):
        self.assertEqual(device_http_status("accepted"), 204)
        self.assertEqual(device_http_status("duplicate"), 204)
        self.assertEqual(device_http_status("stale"), 204)
        self.assertEqual(device_http_status("unknown"), 403)
        self.assertEqual(device_http_status("failed"), 502)

    def test_key_loader_rejects_a_short_key(self):
        with self.assertRaises(ValueError):
            load_key("abcd")


class CollectorHttp(unittest.TestCase):
    def setUp(self):
        ncu_inbox.Handler.key = KEY
        ncu_inbox.Handler.ingest_url = "https://collector.test/ncu/ingest"
        ncu_inbox.Handler.ingest_secret = "station-secret"
        ncu_inbox.Handler.print_only = False
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), ncu_inbox.Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.original = ncu_inbox.post_convex

    def tearDown(self):
        ncu_inbox.post_convex = self.original
        self.server.shutdown()
        self.server.server_close()

    def post(self, blob):
        port = self.server.server_address[1]
        request = urllib.request.Request(
            f"http://127.0.0.1:{port}/ncu/v1/batch",
            data=blob,
            method="POST",
        )
        try:
            with urllib.request.urlopen(request, timeout=2) as response:
                return response.status
        except urllib.error.HTTPError as exc:
            return exc.code

    def test_accepted_batch_is_acknowledged_without_logging_the_key(self):
        seen = {}

        def fake_post(url, secret, body):
            seen["url"] = url
            seen["secret"] = secret
            seen["identity"] = body["identity"]
            seen["count"] = len(body["samples"])
            seen["kw"] = body["samples"][0]["kw100"]
            return 200, "accepted"

        ncu_inbox.post_convex = fake_post
        blob = seal(plain(4, [sample(None, -45)]), KEY, MAC, 4, NONCE)
        output = io.StringIO()
        with redirect_stdout(output):
            status = self.post(blob)
        self.assertEqual(status, 204)
        self.assertEqual(seen["identity"], "aabbccddeeff")
        self.assertEqual(seen["kw"], -45)
        self.assertNotIn(KEY.hex(), output.getvalue())
        self.assertNotIn("station-secret", output.getvalue())
        self.assertNotIn("-45", output.getvalue())

    def test_unknown_device_is_not_acknowledged(self):
        ncu_inbox.post_convex = lambda url, secret, body: (404, "unknown")
        blob = seal(plain(1, [sample(1, None)]), KEY, MAC, 1, NONCE)
        self.assertEqual(self.post(blob), 403)

    def test_convex_failure_is_not_acknowledged(self):
        def broken(url, secret, body):
            raise TimeoutError("timed out")

        ncu_inbox.post_convex = broken
        blob = seal(plain(1, [sample(1, None)]), KEY, MAC, 1, NONCE)
        self.assertEqual(self.post(blob), 502)


if __name__ == "__main__":
    unittest.main()
