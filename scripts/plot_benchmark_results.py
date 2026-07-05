#!/usr/bin/env python3
import re
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
from matplotlib.lines import Line2D

# Result block header, e.g. "thread pool 32 workgroup size - 00_overview:"
HEADER_RE = re.compile(r"^(?P<method>\S.*?) - (?P<zoom>\d+_\w+):\s*$")
# Metric line, e.g. "        median: 60.0264ms"
METRIC_RE = re.compile(
    r"^\s*(?P<key>median|min|max|IQR):\s*(?P<value>[0-9.]+)\s*ms\s*$")


def find_results() -> Path:
    candidates = [
        Path(__file__).resolve().parent / ".." / "results" / "results.txt",
        Path("../results/results.txt"),
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit("results.txt not found; looked in:\n  " +
                     "\n  ".join(str(c.resolve()) for c in candidates))


def parse_results(path: Path) -> dict[str, dict[str, dict[str, float]]]:
    """{method: {zoom: {"median"|"min"|"max"|"IQR": value_ms}}} in file order."""
    data: dict[str, dict[str, dict[str, float]]] = {}
    current = None

    for line in path.read_text().splitlines():
        header = HEADER_RE.match(line)
        if header:
            method, zoom = header["method"], header["zoom"]
            data.setdefault(method, {})[zoom] = {}
            current = (method, zoom)
            continue
        metric = METRIC_RE.match(line)
        if metric and current is not None:
            method, zoom = current
            data[method][zoom][metric["key"]] = float(metric["value"])
    return data


def log_ticks(lo: float, hi: float) -> list[float]:
    """Doubling tick positions (10, 20, 40, ...) covering [lo, hi]."""
    ticks, t = [], 10.0
    while t < hi * 1.4:
        if t > lo * 0.55:
            ticks.append(t)
        t *= 2.0
    return ticks or [lo, hi]


def main() -> None:
    path = find_results()
    data = parse_results(path)
    if not data:
        raise SystemExit(f"no benchmark blocks found in {path}")

    methods = list(data)
    zooms = sorted({z for per_zoom in data.values() for z in per_zoom})
    x = np.arange(len(zooms))
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    offsets = (np.linspace(-0.18, 0.18, len(methods))
               if len(methods) > 1 else [0.0])

    two_methods = len(methods) == 2
    if two_methods:
        fig, (ax, ax_ratio) = plt.subplots(
            2, 1, figsize=(13, 8), sharex=True,
            gridspec_kw={"height_ratios": [3, 1]})
    else:
        fig, ax = plt.subplots(figsize=(13, 6))
        ax_ratio = None

    # --- top panel: median dot, min-max whiskers, IQR band ------------------
    for method, off, color in zip(methods, offsets, colors):
        per_zoom = data[method]
        med = np.array([per_zoom.get(z, {}).get("median", np.nan) for z in zooms])
        lo = np.array([per_zoom.get(z, {}).get("min", np.nan) for z in zooms])
        hi = np.array([per_zoom.get(z, {}).get("max", np.nan) for z in zooms])
        iqr = np.array([per_zoom.get(z, {}).get("IQR", np.nan) for z in zooms])

        ax.errorbar(x + off, med, yerr=[med - lo, hi - med],
                    fmt="o", ms=5, capsize=4, elinewidth=1.2,
                    color=color, label=method, zorder=3)
        has_iqr = ~np.isnan(iqr)
        if has_iqr.any():
            ax.errorbar((x + off)[has_iqr], med[has_iqr],
                        yerr=iqr[has_iqr] / 2.0, fmt="none",
                        elinewidth=7, alpha=0.30, color=color, zorder=2)

    all_vals = [d[k] for per_zoom in data.values() for d in per_zoom.values()
                for k in ("min", "median", "max") if k in d]
    ax.set_yscale("log")
    ax.set_yticks(log_ticks(min(all_vals), max(all_vals)))
    ax.yaxis.set_major_formatter(mticker.FuncFormatter(lambda v, _: f"{v:g}"))
    ax.yaxis.set_minor_formatter(mticker.NullFormatter())
    ax.set_ylabel("frame time (ms, log scale)")
    ax.set_title(f"Mandelbrot benchmark — {path.name}")
    ax.grid(True, which="both", axis="y", alpha=0.3)
    ax.set_xlim(-0.5, len(zooms) - 0.5)

    proxies = [
        Line2D([], [], color="gray", lw=1.2, label="min-max whiskers"),
        Line2D([], [], color="gray", lw=7, alpha=0.30,
               label="IQR band (median ± IQR/2)"),
    ]
    handles, _ = ax.get_legend_handles_labels()
    ax.legend(handles=handles + proxies, fontsize=9, loc="upper left")

    # --- bottom panel: median ratio per zoom ---------------------------------
    if two_methods:
        a, b = methods
        ratio = np.array([
            data[b].get(z, {}).get("median", np.nan) /
            data[a].get(z, {}).get("median", np.nan)
            for z in zooms])
        bar_colors = [colors[0] if r > 1.0 else colors[1] for r in ratio]
        ax_ratio.bar(x, ratio, width=0.55, color=bar_colors, alpha=0.8)
        ax_ratio.axhline(1.0, color="black", lw=1.0, ls="--")
        for xi, r in zip(x, ratio):
            if np.isnan(r):
                continue
            ax_ratio.annotate(f"{r:.2f}x", (xi, r),
                              xytext=(0, 3 if r >= 1.0 else -10),
                              textcoords="offset points", ha="center",
                              va="bottom", fontsize=8)
        ax_ratio.set_ylim(0, max(1.1, float(np.nanmax(ratio)) * 1.2))
        ax_ratio.set_ylabel("median ratio")
        ax_ratio.set_title(
            f"median('{b}') / median('{a}')  —  bar > 1 means '{a}' is faster "
            "(bar colored by the faster method)", fontsize=9)
        ax_ratio.set_xticks(x)
        ax_ratio.set_xticklabels(zooms, rotation=25, ha="right")
        ax_ratio.grid(True, axis="y", alpha=0.3)
    else:
        ax.set_xticks(x)
        ax.set_xticklabels(zooms, rotation=25, ha="right")

    fig.tight_layout()
    out = Path(__file__).resolve().parent / "benchmark_results.png"
    fig.savefig(out, dpi=150)
    print(f"saved {out}")
    plt.show()


if __name__ == "__main__":
    main()