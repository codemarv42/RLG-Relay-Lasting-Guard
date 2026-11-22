# Changelog

## RLG OS 1.2.0 – public release

Cleanup of firmware 1.1.4, which was used for all valid measurements. **The switching and test timing is
unchanged** (cooldown, ON time, zero-crossing wait, failure checks), so results remain comparable.

- Code split into modules (`relay_test`, `measure`, `storage`, `display`, `input`, `serial_ui`, `timekeeping`),
  duplicated per-relay code replaced by a `TestChannel` table, all parameters in `config.h`.
- State backup: `rlg_state.bin` was reopened in truncating mode for every field, so only the last value
  survived. It is now written in one pass and checked for completeness on load.
- Test timers use a 64-bit uptime (no overflow after 49 days) and are only stopped if running, so a disabled
  channel no longer gets a bogus elapsed time.
- Wall clock with correct month / leap-year rollover.
- Switch-time measurement aborts after 50 ms without contact movement instead of hanging (new error 0x16).
- ZC oscilloscope mode now uses the same compensated zero-crossing switching as the test.
- Temperature sensor order configurable (`TEMP_IDX_*`).
- Removed a mutex release without prior lock when resetting a channel.
- Serial menu and display texts in English.
- Licensing clarified: CC BY-NC-SA 4.0, commercial use by agreement (see LICENSING.md).

## RLG OS 1.1.4

Firmware used for relays #22–#45 of the thesis.

## RLG OS ≤ 1.1.3

Development versions with different cycle timing; results not comparable.
