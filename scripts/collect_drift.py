#!/usr/bin/env python3
"""
IMU static drift data collector.

Reads all 50 Hz serial output from the ESP32 IMU node, tracks publishing
reliability for both IMUs, and writes a downsampled CSV (default: one row
per IMU every 10 s) for offline drift analysis.

Usage:
    python collect_drift.py --port COM3
    python collect_drift.py --port COM3 --log-interval 5 --output my_run.csv
    python collect_drift.py --port COM3 --duration 14400   # 4-hour run
"""

import argparse
import csv
import re
import signal
import sys
import time
from collections import defaultdict
from datetime import datetime, timezone


# Serial line format produced by 4a_ser_comms.ino:
#   IMU ID: 0 | Acceleration (m/s^2): X=0.01 Y=-0.02 Z=0.00 | Gyro (rad/s): R=0.001 P=-0.000 Y=0.002
_LINE_RE = re.compile(
    r"IMU ID:\s*(\d+)"
    r"\s*\|\s*Acceleration \(m/s\^2\):\s*X=([+-]?\d+\.\d+)\s+Y=([+-]?\d+\.\d+)\s+Z=([+-]?\d+\.\d+)"
    r"\s*\|\s*Gyro \(rad/s\):\s*R=([+-]?\d+\.\d+)\s+P=([+-]?\d+\.\d+)\s+Y=([+-]?\d+\.\d+)"
)

NUM_IMUS = 2
BAUD_RATE = 115200
STATUS_INTERVAL_S = 60  # how often to print reliability stats to console


def parse_args():
    p = argparse.ArgumentParser(description="IMU drift data collector")
    p.add_argument("--port", required=True, help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    p.add_argument("--baud", type=int, default=BAUD_RATE, help="Baud rate (default 115200)")
    p.add_argument("--log-interval", type=float, default=10.0,
                   help="Seconds between CSV rows per IMU (default 10)")
    p.add_argument("--duration", type=float, default=None,
                   help="Stop after this many seconds (default: run until Ctrl-C)")
    p.add_argument("--output", default=None,
                   help="Output CSV path (default: drift_YYYYMMDD_HHMMSS.csv)")
    p.add_argument("--verbose", action="store_true",
                   help="Print every received packet to the terminal")
    return p.parse_args()


OUTPUT_DIR = "output"


def make_output_path():
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"{OUTPUT_DIR}/drift_{ts}.csv"


def main():
    args = parse_args()

    try:
        import serial
    except ImportError:
        sys.exit("pyserial not installed — run: pip install pyserial")

    output_path = args.output or make_output_path()

    # State per IMU
    packet_counts   = defaultdict(int)   # total packets received
    last_log_time   = defaultdict(float) # wall-clock time of last CSV write
    latest_sample   = {}                  # most recent parsed sample

    # Publishing reliability tracking
    # Expected packets: 50 Hz × elapsed window
    window_start_time  = time.monotonic()
    window_pkt_counts  = defaultdict(int)
    last_status_time   = time.monotonic()

    start_time = time.monotonic()

    shutdown = [False]
    def _sig_handler(sig, frame):
        shutdown[0] = True
    signal.signal(signal.SIGINT, _sig_handler)
    signal.signal(signal.SIGTERM, _sig_handler)

    print(f"Opening {args.port} at {args.baud} baud …")
    print(f"Logging to: {output_path}")
    print(f"CSV log interval: {args.log_interval} s  |  Status every: {STATUS_INTERVAL_S} s")
    if args.duration:
        print(f"Planned duration: {args.duration} s")
    print("Press Ctrl-C to stop.\n")

    with open(output_path, "w", newline="") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(["timestamp_s", "imu_id", "ax", "ay", "az", "groll", "gpitch", "gyaw"])

        with serial.Serial(args.port, args.baud, timeout=1) as ser:
            # Signal the ESP32 to start streaming (sets COMMS_RUNNING_FLAG_BIT)
            time.sleep(0.5)  # brief settle after port open
            ser.write(b'1')
            print("Sent start command to ESP32.")

            try:
              while not shutdown[0]:
                now = time.monotonic()

                # Duration limit
                if args.duration and (now - start_time) >= args.duration:
                    print(f"\nDuration limit ({args.duration} s) reached — stopping.")
                    break

                # Print reliability stats periodically
                if now - last_status_time >= STATUS_INTERVAL_S:
                    elapsed_window = now - window_start_time
                    expected = int(50 * elapsed_window)  # 50 Hz
                    parts = []
                    for imu_id in range(NUM_IMUS):
                        got = window_pkt_counts[imu_id]
                        pct = 100.0 * got / expected if expected > 0 else 0.0
                        parts.append(f"IMU{imu_id}: {got}/{expected} pkts ({pct:.1f}%)")
                    ts_str = datetime.now().strftime("%H:%M:%S")
                    print(f"[{ts_str}] " + " | ".join(parts))

                    # Reset window counters
                    window_pkt_counts.clear()
                    window_start_time = now
                    last_status_time  = now

                # Read a line
                try:
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="replace").strip()
                except serial.SerialException as exc:
                    print(f"Serial error: {exc}")
                    break

                m = _LINE_RE.match(line)
                if not m:
                    continue

                imu_id = int(m.group(1))
                sample = {
                    "timestamp_s": time.time(),
                    "imu_id":      imu_id,
                    "ax":          float(m.group(2)),
                    "ay":          float(m.group(3)),
                    "az":          float(m.group(4)),
                    "groll":       float(m.group(5)),
                    "gpitch":      float(m.group(6)),
                    "gyaw":        float(m.group(7)),
                }

                packet_counts[imu_id]        += 1
                window_pkt_counts[imu_id]    += 1
                latest_sample[imu_id]         = sample

                if args.verbose:
                    ts = datetime.fromtimestamp(sample["timestamp_s"]).strftime("%H:%M:%S.%f")[:-3]
                    print(
                        f"[{ts}] IMU{imu_id} | "
                        f"Accel: X={sample['ax']:8.4f}  Y={sample['ay']:8.4f}  Z={sample['az']:8.4f} m/s² | "
                        f"Gyro:  R={sample['groll']:8.4f}  P={sample['gpitch']:8.4f}  Y={sample['gyaw']:8.4f} rad/s"
                    )

                # Downsample to CSV (log_interval=0 means log every packet)
                if args.log_interval == 0 or now - last_log_time[imu_id] >= args.log_interval:
                    writer.writerow([
                        f"{sample['timestamp_s']:.3f}",
                        imu_id,
                        sample["ax"], sample["ay"], sample["az"],
                        sample["groll"], sample["gpitch"], sample["gyaw"],
                    ])
                    csv_file.flush()
                    last_log_time[imu_id] = now

            # Signal the ESP32 to stop streaming (clears COMMS_RUNNING_FLAG_BIT)
            ser.write(b'2')

    # Final summary
    total_elapsed = time.monotonic() - start_time
    print(f"\n--- Collection complete ({total_elapsed:.0f} s) ---")
    for imu_id in range(NUM_IMUS):
        got      = packet_counts[imu_id]
        expected = int(50 * total_elapsed)
        pct      = 100.0 * got / expected if expected > 0 else 0.0
        print(f"  IMU{imu_id}: {got}/{expected} total packets received ({pct:.2f}%)")
    print(f"Output: {output_path}")


if __name__ == "__main__":
    main()
