#!/usr/bin/env python3
# Copyright (c) 2024-2026 Marvin Ramm - CC BY-NC-SA 4.0, see LICENSING.md
"""Reproduces the summary statistics and figures of the relay lifetime study.

Usage:  python data/scripts/analyze.py          (from the repository root)
Needs:  pandas, matplotlib
Writes: docs/images/lifetime_by_group.png, survival.png, contact_resistance.png
"""
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "results.csv"
OUT = ROOT / "docs" / "images"

DATASHEET_CYCLES = 50_000  # APAN3105, 5 A resistive
SURFACE, INK, INK_2, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"
COLOR = {"off": "#eb6834", "on": "#2a78d6"}          # categorical slots 2 / 1
LABEL = {"off": "Random switching", "on": "Zero-crossing switching"}
GROUPS = [("resistive", "off"), ("resistive", "on"), ("capacitive", "off"), ("capacitive", "on")]

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": INK_2, "axes.labelcolor": INK_2, "xtick.color": INK_2, "ytick.color": INK_2,
    "text.color": INK, "font.size": 11, "axes.spines.top": False, "axes.spines.right": False,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8, "axes.axisbelow": True,
    "legend.frameon": False,
})


def load() -> pd.DataFrame:
    df = pd.read_csv(DATA)
    return df[(df["in_lifetime_stats"] == "yes")].copy()


def mean_abs_dev(s: pd.Series) -> float:
    return (s - s.mean()).abs().mean()


def summary(df: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for load_type, zc in GROUPS:
        g = df[(df.load == load_type) & (df.zero_cross == zc)]
        rows.append({
            "group": f"{load_type}, ZC {zc}", "n": len(g),
            "stopped_without_failure": int((g.end_of_test == "stopped").sum()),
            "mean_cycles": g.cycles.mean(), "mean_abs_dev": mean_abs_dev(g.cycles),
            "rc_start_mohm": g.contact_res_start_mohm.mean(), "rc_end_mohm": g.contact_res_end_mohm.mean(),
        })
    s = pd.DataFrame(rows)
    s["rc_increase_mohm"] = s.rc_end_mohm - s.rc_start_mohm
    return s


def plot_lifetime(df: pd.DataFrame, s: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(8, 5.2), dpi=150)
    xs = {g: i for i, g in enumerate(GROUPS)}
    means = []
    for (load_type, zc), x in xs.items():
        g = df[(df.load == load_type) & (df.zero_cross == zc)]
        m = g.cycles.mean()
        ax.bar(x, m, width=0.55, color=COLOR[zc], alpha=0.25, edgecolor=COLOR[zc], linewidth=1.5)
        for k, (_, r) in enumerate(g.iterrows()):
            dx = (k - (len(g) - 1) / 2) * 0.09
            stopped = r.end_of_test == "stopped"
            ax.plot(x + dx, r.cycles, marker="^" if stopped else "o", markersize=8,
                    markerfacecolor=SURFACE if stopped else COLOR[zc], markeredgecolor=COLOR[zc], markeredgewidth=2)
        prefix = ">= " if (g.end_of_test == "stopped").any() else ""
        means.append(f"mean {prefix}{m:,.0f}".replace(",", " "))
    ax.axhline(DATASHEET_CYCLES, color=INK_2, linestyle="--", linewidth=1)
    ax.text(3.45, DATASHEET_CYCLES * 1.12, "datasheet 50 000", color=INK_2, fontsize=9, ha="right")
    for load_type, i0 in (("resistive", 0), ("capacitive", 2)):
        on = s[s.group == f"{load_type}, ZC on"].mean_cycles.iloc[0]
        off = s[s.group == f"{load_type}, ZC off"].mean_cycles.iloc[0]
        ax.text(i0 + 0.5, 3e6, f">= {on / off:.0f}x", ha="center", fontsize=12, fontweight="bold", color=INK)
    ax.set_yscale("log")
    ax.set_ylim(150, 6e6)
    names = ["Resistive\nrandom", "Resistive\nzero-crossing", "Capacitive\nrandom", "Capacitive\nzero-crossing"]
    ax.set_xticks(range(4), [f"{n}\n{m}" for n, m in zip(names, means)])
    ax.set_ylabel("Switching cycles until failure")
    ax.grid(axis="x", visible=False)
    handles = [plt.Line2D([], [], marker="o", ls="", color=INK_2, markersize=7, label="failed (welded)"),
               plt.Line2D([], [], marker="^", ls="", markerfacecolor=SURFACE, markeredgecolor=INK_2, markersize=7,
                          markeredgewidth=2, label="stopped without failure")]
    ax.legend(handles=handles, loc="upper center", bbox_to_anchor=(0.5, -0.24), ncol=2, fontsize=9)
    ax.set_title("APAN3105 switching lifetime, 230 V AC, 5.5 A", loc="left", fontsize=12, color=INK)
    fig.tight_layout()
    fig.savefig(OUT / "lifetime_by_group.png")
    plt.close(fig)


def kaplan_meier(cycles: pd.Series, failed: pd.Series):
    """Kaplan-Meier estimate; returns step coordinates and censoring points."""
    d = pd.DataFrame({"t": cycles, "f": failed}).sort_values("t")
    at_risk, s = len(d), 1.0
    xs, ys, cens = [0], [1.0], []
    for _, r in d.iterrows():
        if r.f:
            xs += [r.t, r.t]
            ys += [s, s * (1 - 1 / at_risk)]
            s *= 1 - 1 / at_risk
        else:
            cens.append((r.t, s))
        at_risk -= 1
    return xs, ys, cens


def plot_survival(df: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(8, 4.2), dpi=150)
    for load_type, zc in GROUPS:
        g = df[(df.load == load_type) & (df.zero_cross == zc)]
        xs, ys, cens = kaplan_meier(g.cycles, g.end_of_test == "welded")
        xs = [max(x, 100) for x in xs]
        end = g.cycles.max()
        xs.append(end); ys.append(ys[-1])
        ls = "-" if load_type == "resistive" else "--"
        ax.step(xs, [y * 100 for y in ys], where="post", color=COLOR[zc], linewidth=2, linestyle=ls,
                label=f"{load_type.capitalize()}, {LABEL[zc].lower()}")
        for t, sv in cens:
            ax.plot(t, sv * 100, marker="|", markersize=12, markeredgewidth=2, color=COLOR[zc])
    ax.axvline(DATASHEET_CYCLES, color=INK_2, linestyle=":", linewidth=1)
    ax.text(DATASHEET_CYCLES * 1.08, 4, "datasheet", color=INK_2, fontsize=9)
    ax.set_xscale("log")
    ax.set_xlim(100, 2e6)
    ax.set_ylim(-3, 105)
    ax.set_xlabel("Switching cycles")
    ax.set_ylabel("Relays still working (%)")
    ax.legend(loc="lower left", fontsize=9)
    ax.set_title("Survival per test group (| = stopped without failure)", loc="left", fontsize=12, color=INK)
    fig.tight_layout()
    fig.savefig(OUT / "survival.png")
    plt.close(fig)


def plot_contact_resistance(s: pd.DataFrame) -> None:
    fig, ax = plt.subplots(figsize=(8, 3.8), dpi=150)
    w = 0.36
    for i, r in s.iterrows():
        zc = "on" if r.group.endswith("on") else "off"
        ax.bar(i - w / 2 - 0.01, r.rc_start_mohm, w, color=COLOR[zc], alpha=0.3)
        ax.bar(i + w / 2 + 0.01, r.rc_end_mohm, w, color=COLOR[zc])
        ax.annotate(f"+{r.rc_increase_mohm:.2f}", (i + w / 2, r.rc_end_mohm), xytext=(0, 4),
                    textcoords="offset points", ha="center", fontsize=9, color=INK)
    ax.set_xticks(range(4), ["Resistive\nrandom", "Resistive\nzero-crossing", "Capacitive\nrandom", "Capacitive\nzero-crossing"])
    ax.set_ylabel("Contact resistance (mOhm)")
    ax.grid(axis="x", visible=False)
    handles = [plt.Rectangle((0, 0), 1, 1, color=INK_2, alpha=0.3, label="before test"),
               plt.Rectangle((0, 0), 1, 1, color=INK_2, label="after test")]
    ax.legend(handles=handles, loc="upper right", fontsize=9)
    ax.set_title("Contact resistance, group mean (voltage-drop method, 2.2 A DC)", loc="left", fontsize=12, color=INK)
    fig.tight_layout()
    fig.savefig(OUT / "contact_resistance.png")
    plt.close(fig)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    df = load()
    s = summary(df)
    with pd.option_context("display.width", 140, "display.float_format", "{:,.2f}".format):
        print(s.to_string(index=False))
    for load_type in ("resistive", "capacitive"):
        on = s[s.group == f"{load_type}, ZC on"].mean_cycles.iloc[0]
        off = s[s.group == f"{load_type}, ZC off"].mean_cycles.iloc[0]
        print(f"{load_type}: lifetime gain >= {on / off:.1f}x")
    plot_lifetime(df, s)
    plot_survival(df)
    plot_contact_resistance(s)


if __name__ == "__main__":
    main()
