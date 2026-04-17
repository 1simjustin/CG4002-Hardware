#!/usr/bin/env python3
"""
IMU drift analyser.

Loads a CSV produced by collect_drift.py and generates:
  1. Time-series plot of all 6 channels per IMU
  2. 1-minute rolling mean and ±1 std dev band overlay
  3. Linear drift trend line per channel
  4. Summary statistics table (mean, std, min, max, drift slope)
  5. (--allan) Allan deviation plot per IMU channel
  6. (--gap-check) Report timestamp gaps > 2× log-interval (dropped samples)

Usage:
    python analyze_drift.py drift_20260417_210000.csv
    python analyze_drift.py drift_20260417_210000.csv --allan
    python analyze_drift.py drift_20260417_210000.csv --gap-check
    python analyze_drift.py drift_20260417_210000.csv --imu 0
"""

import argparse
import sys

import numpy as np
import pandas as pd


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    p = argparse.ArgumentParser(description="IMU drift analyser")
    p.add_argument("csv", help="CSV file from collect_drift.py")
    p.add_argument("--imu", type=int, default=None,
                   help="Analyse only this IMU ID (default: all)")
    p.add_argument("--allan", action="store_true",
                   help="Also produce Allan deviation plots")
    p.add_argument("--gap-check", action="store_true",
                   help="Report timestamp gaps indicating dropped samples")
    p.add_argument("--save", action="store_true",
                   help="Save plots to PNG instead of showing interactively")
    return p.parse_args()


# ---------------------------------------------------------------------------
# Data loading
# ---------------------------------------------------------------------------

CHANNELS = ["ax", "ay", "az", "groll", "gpitch", "gyaw"]
CHANNEL_LABELS = {
    "ax": "Accel X (m/s²)",
    "ay": "Accel Y (m/s²)",
    "az": "Accel Z (m/s²)",
    "groll":  "Gyro Roll (rad/s)",
    "gpitch": "Gyro Pitch (rad/s)",
    "gyaw":   "Gyro Yaw (rad/s)",
}

# MPU6050 datasheet-derived pass criteria (world-frame, post-calibration)
PASS_THRESHOLDS = {
    "ax": 0.05, "ay": 0.05, "az": 0.05,       # m/s² std
    "groll": 0.005, "gpitch": 0.005, "gyaw": 0.005,  # rad/s std
}
SLOPE_THRESHOLD = 1e-5  # units/s


def load_data(path, imu_filter=None):
    df = pd.read_csv(path)
    df["timestamp_s"] = pd.to_numeric(df["timestamp_s"], errors="coerce")
    df = df.dropna(subset=["timestamp_s"])
    df = df.sort_values("timestamp_s").reset_index(drop=True)

    if imu_filter is not None:
        df = df[df["imu_id"] == imu_filter]

    # Relative time from first sample
    t0 = df["timestamp_s"].min()
    df["rel_s"] = df["timestamp_s"] - t0
    df["rel_min"] = df["rel_s"] / 60.0
    return df


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def compute_stats(df, imu_id):
    sub = df[df["imu_id"] == imu_id]
    rows = []
    for ch in CHANNELS:
        vals = sub[ch].dropna().values
        if len(vals) < 2:
            continue
        t = sub.loc[sub[ch].notna(), "rel_s"].values
        slope, _ = np.polyfit(t, vals, 1)
        pass_std = abs(vals.std()) < PASS_THRESHOLDS[ch]
        pass_slope = abs(slope) < SLOPE_THRESHOLD
        rows.append({
            "channel": ch,
            "mean":    vals.mean(),
            "std":     vals.std(),
            "min":     vals.min(),
            "max":     vals.max(),
            "slope (units/s)": slope,
            "std OK": "PASS" if pass_std else "FAIL",
            "slope OK": "PASS" if pass_slope else "FAIL",
        })
    return pd.DataFrame(rows).set_index("channel")


def print_stats(stats, imu_id):
    print(f"\n{'='*60}")
    print(f"IMU {imu_id} — Summary Statistics")
    print(f"{'='*60}")
    pd.set_option("display.float_format", "{:.6f}".format)
    pd.set_option("display.max_columns", 10)
    pd.set_option("display.width", 120)
    print(stats.to_string())
    print()


# ---------------------------------------------------------------------------
# Gap check
# ---------------------------------------------------------------------------

def gap_check(df, imu_id):
    sub = df[df["imu_id"] == imu_id].copy()
    if len(sub) < 2:
        print(f"IMU {imu_id}: not enough data for gap check.")
        return
    diffs = sub["timestamp_s"].diff().dropna()
    median_interval = diffs.median()
    threshold = 2.5 * median_interval
    gaps = diffs[diffs > threshold]
    print(f"\nIMU {imu_id} gap check (threshold = {threshold:.2f} s, "
          f"median interval = {median_interval:.2f} s):")
    if gaps.empty:
        print("  No gaps detected — publishing reliability OK.")
    else:
        print(f"  {len(gaps)} gap(s) found:")
        for idx, gap_s in gaps.items():
            t_rel = sub.loc[idx, "rel_min"]
            print(f"    t={t_rel:.2f} min — gap of {gap_s:.2f} s")


# ---------------------------------------------------------------------------
# Allan deviation
# ---------------------------------------------------------------------------

def allan_deviation(data, dt):
    """
    Compute overlapping Allan deviation.
    data: 1-D numpy array of equally spaced samples
    dt:   sample interval in seconds
    Returns (tau_array, adev_array)
    """
    n = len(data)
    max_m = n // 2
    taus, adevs = [], []
    cumsum = np.cumsum(data)

    for m in range(1, max_m + 1):
        # Overlapping AVAR
        avars = []
        for k in range(n - 2 * m):
            theta_k2m = cumsum[k + 2 * m] - cumsum[k + m]  # sum [k+m .. k+2m-1]
            theta_km  = cumsum[k + m]     - cumsum[k]       # sum [k .. k+m-1]
            avars.append((theta_k2m - theta_km) ** 2)
        if avars:
            avar = np.mean(avars) / (2 * m ** 2 * dt ** 2 * (n - 2 * m))
            taus.append(m * dt)
            adevs.append(np.sqrt(avar))

    return np.array(taus), np.array(adevs)


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def plot_timeseries(df, imu_id, save=False):
    import matplotlib.pyplot as plt
    import matplotlib.gridspec as gridspec

    sub = df[df["imu_id"] == imu_id].copy()
    if sub.empty:
        print(f"IMU {imu_id}: no data to plot.")
        return

    fig = plt.figure(figsize=(14, 10))
    fig.suptitle(f"IMU {imu_id} — Static Drift ({sub['rel_min'].max():.1f} min)", fontsize=13)
    gs  = gridspec.GridSpec(2, 3, figure=fig, hspace=0.45, wspace=0.35)

    for idx, ch in enumerate(CHANNELS):
        ax = fig.add_subplot(gs[idx // 3, idx % 3])
        t  = sub["rel_min"].values
        y  = sub[ch].values

        # Raw trace
        ax.plot(t, y, linewidth=0.6, alpha=0.7, color="steelblue", label="raw")

        # Rolling mean ± std (1-minute window based on rows)
        s = pd.Series(y)
        rows_per_min = max(1, int(60 / (sub["rel_s"].diff().median() or 10)))
        roll_mean = s.rolling(rows_per_min, center=True).mean()
        roll_std  = s.rolling(rows_per_min, center=True).std()
        ax.plot(t, roll_mean.values, color="crimson", linewidth=1.2, label="1-min mean")
        ax.fill_between(t,
                        (roll_mean - roll_std).values,
                        (roll_mean + roll_std).values,
                        alpha=0.2, color="crimson", label="±1σ")

        # Trend line
        coeffs = np.polyfit(t * 60, y, 1)   # slope in units/s
        trend  = np.polyval(coeffs, t * 60)
        ax.plot(t, trend, "--", color="darkorange", linewidth=1.0,
                label=f"trend {coeffs[0]:.2e} u/s")

        ax.set_xlabel("Time (min)", fontsize=8)
        ax.set_ylabel(CHANNEL_LABELS[ch], fontsize=8)
        ax.tick_params(labelsize=7)
        ax.legend(fontsize=6, loc="upper right")

        threshold = PASS_THRESHOLDS[ch]
        pass_ok = abs(y.std()) < threshold
        color = "green" if pass_ok else "red"
        ax.set_title(CHANNEL_LABELS[ch].split(" (")[0], fontsize=9, color=color)

    plt.tight_layout()
    if save:
        fname = f"drift_timeseries_imu{imu_id}.png"
        plt.savefig(fname, dpi=150)
        print(f"Saved {fname}")
    else:
        plt.show()
    plt.close()


def plot_allan(df, imu_id, save=False):
    import matplotlib.pyplot as plt

    sub = df[df["imu_id"] == imu_id].copy()
    if sub.empty:
        print(f"IMU {imu_id}: no data for Allan deviation.")
        return

    # Infer sample interval from median time delta
    dt = sub["rel_s"].diff().median()
    if np.isnan(dt) or dt <= 0:
        print(f"IMU {imu_id}: cannot infer sample interval for Allan deviation.")
        return

    fig, axes = plt.subplots(2, 3, figsize=(14, 8))
    fig.suptitle(f"IMU {imu_id} — Allan Deviation", fontsize=13)

    for idx, ch in enumerate(CHANNELS):
        ax   = axes[idx // 3][idx % 3]
        data = sub[ch].dropna().values

        if len(data) < 10:
            ax.set_title(ch + " (insufficient data)", fontsize=9)
            continue

        taus, adevs = allan_deviation(data, dt)
        ax.loglog(taus, adevs, linewidth=1.2, color="steelblue")

        # Reference slopes
        t_ref = np.array([taus[0], taus[-1]])
        mid_tau = np.exp(np.log(taus).mean())
        mid_adev = np.exp(np.log(adevs).mean())

        ax.loglog(t_ref, mid_adev * (t_ref / mid_tau) ** -0.5,
                  "--", color="gray", linewidth=0.8, label="slope −½ (white noise)")
        ax.loglog(t_ref, mid_adev * (t_ref / mid_tau) ** 0.5,
                  ":", color="gray", linewidth=0.8, label="slope +½ (rate ramp)")

        ax.set_xlabel("τ (s)", fontsize=8)
        ax.set_ylabel("ADEV", fontsize=8)
        ax.set_title(CHANNEL_LABELS[ch].split(" (")[0], fontsize=9)
        ax.tick_params(labelsize=7)
        ax.legend(fontsize=6)
        ax.grid(True, which="both", linestyle=":", alpha=0.5)

    plt.tight_layout()
    if save:
        fname = f"drift_allan_imu{imu_id}.png"
        plt.savefig(fname, dpi=150)
        print(f"Saved {fname}")
    else:
        plt.show()
    plt.close()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    args = parse_args()

    try:
        import matplotlib  # noqa: F401
    except ImportError:
        sys.exit("matplotlib not installed — run: pip install matplotlib")

    df = load_data(args.csv, imu_filter=args.imu)
    if df.empty:
        sys.exit("No data loaded — check CSV path and IMU ID filter.")

    imu_ids = sorted(df["imu_id"].unique())
    duration_min = df["rel_s"].max() / 60.0
    print(f"Loaded {len(df)} rows, IMU(s): {imu_ids}, duration: {duration_min:.1f} min")

    for imu_id in imu_ids:
        stats = compute_stats(df, imu_id)
        print_stats(stats, imu_id)

        if args.gap_check:
            gap_check(df, imu_id)

        plot_timeseries(df, imu_id, save=args.save)

        if args.allan:
            plot_allan(df, imu_id, save=args.save)


if __name__ == "__main__":
    main()
