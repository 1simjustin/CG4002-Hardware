# IMU Diagnostics Scripts

Python scripts for collecting and analysing MPU6050 IMU data from the CG4002 ESP32 node over serial.

## Requirements

```
pip install pyserial pandas numpy matplotlib
```

---

## `collect_drift.py` — Serial Data Logger

Reads the 50 Hz serial output from the ESP32, tracks publishing reliability for both IMUs, and writes a downsampled CSV for offline drift analysis.

### Arguments

| Argument | Default | Description |
|---|---|---|
| `--port` | *(required)* | Serial port (e.g. `COM3`, `/dev/ttyUSB0`) |
| `--baud` | `115200` | Baud rate |
| `--log-interval` | `10` | Seconds between CSV rows per IMU |
| `--duration` | *(none)* | Stop after this many seconds. Omit to run until Ctrl-C |
| `--output` | `drift_YYYYMMDD_HHMMSS.csv` | Output CSV path |

### Usage

```bash
# Basic — runs until Ctrl-C, logs every 10 s
python collect_drift.py --port COM3

# Log every 5 s, stop after 4 hours, custom output file
python collect_drift.py --port COM3 --log-interval 5 --duration 14400 --output overnight.csv
```

### Output

**CSV columns:** `timestamp_s, imu_id, ax, ay, az, groll, gpitch, gyaw`

- `ax/ay/az` — world-frame linear acceleration in m/s² (gravity removed)
- `groll/gpitch/gyaw` — world-frame angular velocity in rad/s

**Console (every 60 s):**
```
[21:05:00] IMU0: 2998/3000 pkts (99.9%) | IMU1: 3000/3000 pkts (100.0%)
```

**Final summary on exit:**
```
--- Collection complete (3600 s) ---
  IMU0: 179988/180000 total packets received (99.99%)
  IMU1: 180000/180000 total packets received (100.00%)
Output: drift_20260417_210000.csv
```

### Static Drift Test Setup

1. Power the ESP32 and keep the node **completely still** during the boot calibration (~1 s).
2. Place the node on a flat, vibration-free surface.
3. Start the script and leave undisturbed for **4–8 hours**.

---

## `analyze_drift.py` — Post-Collection Analyser

Loads a CSV from `collect_drift.py` and produces statistics and plots.

### Arguments

| Argument | Default | Description |
|---|---|---|
| `csv` | *(required)* | CSV file from `collect_drift.py` |
| `--imu` | *(all)* | Analyse only this IMU ID (`0` or `1`) |
| `--allan` | off | Also produce Allan deviation plots |
| `--gap-check` | off | Report timestamp gaps indicating dropped samples |
| `--save` | off | Save plots as PNG files instead of showing interactively |

### Usage

```bash
# Basic analysis — summary table + time-series plots
python analyze_drift.py drift_20260417_210000.csv

# Full analysis — include Allan deviation and gap check
python analyze_drift.py drift_20260417_210000.csv --allan --gap-check

# Analyse only IMU 0, save plots to PNG
python analyze_drift.py drift_20260417_210000.csv --imu 0 --save
```

### Outputs

#### Summary Statistics Table

Printed to console for each IMU. Columns: `mean`, `std`, `min`, `max`, `slope (units/s)`, and PASS/FAIL verdicts against the thresholds below.

| Channel | Std threshold | Slope threshold |
|---|---|---|
| Accel X/Y/Z | < 0.05 m/s² | < 1×10⁻⁵ units/s |
| Gyro Roll/Pitch/Yaw | < 0.005 rad/s | < 1×10⁻⁵ units/s |

#### Time-Series Plot (`drift_timeseries_imuN.png`)

One subplot per channel showing:
- Raw trace
- 1-minute rolling mean and ±1σ band
- Linear drift trend line with slope annotation
- Green/red title indicating PASS/FAIL

#### Allan Deviation Plot (`drift_allan_imuN.png`) — `--allan`

Log-log Allan deviation per channel. Reference slope lines are overlaid:
- Slope −½ → white noise (angle/velocity random walk)
- Slope 0 minimum → bias instability
- Slope +½ → rate random walk / long-term drift

#### Gap Check — `--gap-check`

Reports any CSV timestamp intervals larger than 2.5× the median interval, indicating dropped or delayed serial packets.

```
IMU 0 gap check (threshold = 25.00 s, median interval = 10.01 s):
  No gaps detected — publishing reliability OK.
```

---

## Typical Workflow

```bash
# 1. Collect overnight (8 hours)
python collect_drift.py --port COM3 --duration 28800 --output overnight.csv

# 2. Quick summary
python analyze_drift.py overnight.csv

# 3. Full diagnostics with Allan deviation
python analyze_drift.py overnight.csv --allan --gap-check --save
```
