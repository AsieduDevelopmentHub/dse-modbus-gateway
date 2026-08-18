"""Host-side DSE watch loop. READ ONLY. Never sends scf stop."""
import sys
import time
import serial

PORT = "COM3"
BAUD = 115200
CYCLES = 2
GAP_S = 2.0

# Hard denylist — must never be sent while DG is running
FORBIDDEN = (
    "scf stop",
    "scf raw 35700",
    "scf raw 0x8b74",
    "telemetry_stop",
)

READ_CMDS = [
    "status",
    "rs485raw 1 03 0405 1",
    "iterate basic",
    "scf flags",
]


def drain(ser, seconds=0.4):
    t0 = time.time()
    chunks = []
    while time.time() - t0 < seconds:
        n = ser.in_waiting
        if n:
            chunks.append(ser.read(n))
            t0 = time.time()
        else:
            time.sleep(0.05)
    return b"".join(chunks)


def send_cmd(ser, cmd, wait=8.0):
    low = cmd.strip().lower()
    for bad in FORBIDDEN:
        if bad in low:
            print("BLOCKED forbidden command:", cmd)
            return ""
    ser.reset_input_buffer()
    ser.write((cmd + "\r\n").encode("ascii"))
    ser.flush()
    t0 = time.time()
    buf = b""
    while time.time() - t0 < wait:
        n = ser.in_waiting
        if n:
            buf += ser.read(n)
            if buf.endswith(b"> ") or buf.rstrip().endswith(b">"):
                break
        else:
            time.sleep(0.05)
    return buf.decode("utf-8", errors="replace")


def main():
    print("Opening", PORT, "READ-ONLY watch. SCF stop is blocked.")
    ser = serial.Serial(
        PORT,
        BAUD,
        timeout=1,
        dsrdtr=False,
        rtscts=False,
    )
    try:
        ser.dtr = False
        ser.rts = False
        time.sleep(1.2)
        banner = drain(ser, 1.5)
        if banner:
            print(banner.decode("utf-8", errors="replace"))
        send_cmd(ser, "", wait=1.0)
        hello = send_cmd(ser, "help", wait=4.0)
        print(hello)

        for i in range(1, CYCLES + 1):
            print("\n======== HOST CYCLE %d/%d ========" % (i, CYCLES))
            for cmd in READ_CMDS:
                print("\n>>>", cmd)
                out = send_cmd(ser, cmd, wait=25.0 if cmd.startswith("iterate") or cmd == "alarms" else 12.0)
                print(out)
            if i < CYCLES:
                print("(pause %.1fs)" % GAP_S)
                time.sleep(GAP_S)
        print("\nWATCH complete. No SCF stop was sent.")
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
