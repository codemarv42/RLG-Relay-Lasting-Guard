# Measurement data

## `results.csv`

One row per tested relay.

| Column | Meaning |
|---|---|
| `relay_no`, `relay_id` | specimen number (decimal / hex as used in the log file names) |
| `model`, `slot` | relay model, test socket (1/2) |
| `zero_cross` | `on` = zero-crossing switching, `off` = random switching |
| `load` | `resistive` (40 Ω) or `capacitive` (40 Ω + 150 µF behind the rectifier) |
| `cycles`, `runtime_s` | switching cycles and test time until the end of test |
| `end_of_test` | `welded` = failed, `stopped` = stopped manually without failure (censored) |
| `contact_res_*_mohm` | contact resistance before / after the test (2.2 A DC, voltage-drop method) |
| `t_on_*_us`, `t_off_*_us` | mean operate / release time before / after the test (20 cycles each) |
| `load_temp_c` | steady-state temperature of the load resistor |
| `firmware_valid` | `yes` = tested with firmware ≥ 1.1.4 under identical conditions |
| `in_lifetime_stats` | used for the group statistics (`reference model` = SRD-05VDC-SL-C) |

Relays #3–#21 were tested with earlier firmware versions that used different cycle timing. They are listed for completeness but are **not** used in the analysis.

## `logs/`

Raw SD card logs of the valid runs (#22–#30, file names use the hex number). Format: see [docs/firmware.md](../docs/firmware.md#sd-card).

## `scripts/analyze.py`

```bash
pip install pandas matplotlib
python data/scripts/analyze.py
```

Prints the group statistics and regenerates the figures in `docs/images/`.
