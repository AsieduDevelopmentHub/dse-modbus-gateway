# NCU uplink

The gateway keeps the Modbus bus and the cloud path apart. Sampling copies the last telemetry snapshot into a fixed batch of at most 8 records. Nothing in the uplink opens a Modbus transaction, grows a queue, or keeps a socket open.

## What is sent

One HTTP POST carries the whole batch. The body is `NCU1`: a 30-byte header, AES-128-GCM ciphertext, and a 16-byte tag. The header (including the random 12-byte nonce and the sequence) is authenticated. A repeated nonce is not used; each attempt draws a new nonce. The plaintext is a short JSON list of samples (`rpm`, `batt10` in tenths of a volt, `hz10`, `kw100`, `van10`, mode, alarm count, link).

The POST goes to the station network only. Payload encryption is AES-GCM. Put the collector on the LAN, or on a host that can reach the station. The device does not open a TLS session per batch, because that is what exhausts the ESP32-C3 heap.

## Failure behaviour

- The batch lives in a static array. When it is full, the oldest sample is dropped and `dropped` increments. RAM does not grow.
- A send runs only when the batch reaches `uplink.batch`, the station has an address, and free heap is at least 24 KB.
- Connect and write are capped at about 3 seconds, then the socket is closed. The next try waits 5 s, then 15, 45, 120, 300, and 900 s. A HTTP 2xx clears the batch and resets the wait.
- A failed send keeps the same samples. `uplink now` asks for a send on the next quiet loop even if the batch is not full.
- While telemetry is refreshing the bus, the uplink waits. It does not start a second master.

## Configure

`set telemetry on` is what fills the snapshot. Then:

```text
config set uplink.host 192.168.1.20
config set uplink.port 8080
config set uplink.path /ncu/v1/batch
config set uplink.period_s 30
config set uplink.batch 4
config set uplink.key 00112233445566778899aabbccddeeff
config set uplink.enable on
uplink
```

`uplink` prints batch fill, sequence, sent, failed, dropped, heap, and the last result. The key is stored in `/etc/gateway.conf` and masked in `config`.

Collector on the station side:

```text
python tools/ncu_inbox.py --key 00112233445566778899aabbccddeeff --listen 0.0.0.0:8080
```

That process needs the `cryptography` package. A 204 response is the only ack the device needs.
