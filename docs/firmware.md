# RLG OS – firmware

C++ / Arduino framework on the ESP32 master board. Source: [`firmware/RLG_OS`](../firmware/RLG_OS).

## Architecture

| Core | Task | Responsibilities |
|---|---|---|
| 0 | `RelayTest` (FreeRTOS) | switching cycles of both test relays, failure detection |
| 1 | Arduino `loop()` | encoder, OLED, serial menu, temperature, SD logging, state backup, mains frequency |

Running the test on its own core keeps display updates and SD writes from disturbing the zero-crossing timing.

| File | Content |
|---|---|
| `config.h` | all tunable parameters (timing, limits, logging) |
| `pins.h` | GPIO mapping of the master board |
| `relay_test.*` | test channels, zero-crossing switching, safety test, test task |
| `measure.*` | mains detection, frequency, DS18B20, switch-time measurement |
| `storage.*` | SD card: settings, state backup, logs, firmware update |
| `display.*`, `input.*`, `serial_ui.*` | user interface |
| `timekeeping.*` | 64-bit uptime, per-relay test timers, wall clock |

## Operation

1. Power on. The board asks you to switch on the slave breaker and press the encoder. GPIOs are undefined during boot, so the slave is only powered afterwards.
2. Select a mode:
   - **Lifespan**: continuous test until failure.
   - **Contact Res.**: pauses at 5k, 10k, 15k, 25k, 50k, 100k and 150k cycles, then every 50k, so the contact resistance can be measured.
   - **ZC Oscilloscope**: switches a reference relay in socket 1 (1 s ON / 5 s OFF), free-running or zero-crossing synchronised, for calibrating the correction constants.
3. Set date and time (no RTC on board) and optionally load the previous test state from the SD card.
4. Over the serial menu (115200 baud): set the relay numbers (cmd 2), measure the switch times in the reference socket (cmd 3), and choose zero-crossing on/off per channel (cmd 10).
5. Press the encoder (or cmd 1) to start. The safety test runs first:
   - L/N polarity check
   - safety relay closes and opens
   - each test relay closes and opens

   Only then does the load get power.

The test pauses automatically when a load resistor exceeds 60 °C and resumes below 40 °C.

## Serial menu

| Cmd | Function |
|---|---|
| 1 | start / stop test |
| 2 | set relay number (hex) |
| 3 | measure switch time in the reference socket (save to R1/R2 or view only) |
| 4 | run safety test |
| 5 | live data monitor |
| 6 / 7 | save / load test state |
| 8 | reset channel (log files are kept) |
| 9 | enable / disable channel |
| 10 | toggle zero-crossing switching |
| 11 | set date & time (`DD.MM.YYYY HH:MM:SS`) |
| 12 | restart |

## SD card

| File | Content |
|---|---|
| `/rlg_settings.txt` | relay numbers, switch times, enabled / zero-crossing flags |
| `/rlg_state.bin` | elapsed time, cycles and fails per channel, saved every 60 s to resume after a power loss |
| `/RLG_LOGS/0x<id>_R<n>_Test.txt` | running test log |
| `/RLG_LOGS/DONE_0x<id>_R<n>_Test.txt` | finished log; a relay number with a DONE log cannot be tested again |
| `/firmware.bin` | flashed automatically at boot, then renamed to `firmware.bak` |

Log format: a `#` comment header with all test parameters, followed by tab-separated rows (one per minute) that import directly into a spreadsheet:

```
Timestamp  Date        Elapsed_s  Switches  Fails  Event
10:42:50   12/05/2026  58         26        0
...
03:45:21   22/06/2026  2122163    1302480   40     TEST_END:0x2
```

## Error codes

| Code | Meaning |
|---|---|
| 0x01 | L and N reversed |
| 0x02 / 0x03 | R1 / R2 contact live without activation (welded) |
| 0x05 / 0x06 | R1 / R2 contact did not close |
| 0x07 | fewer than 2 temperature sensors |
| 0x08 | master not on mains |
| 0x09 / 0x0A | load 1 / 2 overtemperature |
| 0x0B / 0x0C | safety relay welded / not closing |
| 0x0D, 0x11, 0x12, 0x15 | SD card errors |
| 0x0E | both channels use the same relay number |
| 0x0F / 0x10 | relay number of R1 / R2 not set |
| 0x13 / 0x14 | DONE log for this relay number already exists |
| 0x16 | switch-time measurement: no contact movement |

## Calibration

`ZC_CORRECTION_ON_US` / `ZC_CORRECTION_OFF_US` in `config.h` depend on the detection circuit. To calibrate:

1. Put a relay into socket 1 and choose *ZC Oscilloscope → ZC-sync*.
2. Show the mains voltage and the voltage behind the contact on a scope, using an isolation transformer.
3. Adjust the constants until closing and opening lie on the zero crossing.

The free-running mode shows the uncompensated behaviour for comparison.
