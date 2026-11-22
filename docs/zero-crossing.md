# Zero-crossing switching for mechanical relays

## Why

An arc forms whenever a contact opens or closes under voltage. It melts and transfers contact material until the contacts weld. If the contact moves while the instantaneous mains voltage is near 0 V, the arc energy drops drastically. For solid-state relays this is standard practice. For mechanical relays it is rarely used, because the contact moves several milliseconds *after* the coil is energised.

## Timing

```
mains   ~~~\        /~~~~\        /~~~
            \______/      \______/
ZC pulse  _|‾|______________|‾|_______
             ^ reference edge
coil      ______|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
             |<-wait->|<--- t_on --->|
contact   ___________________________|‾‾‾  closes at the next zero crossing
```

The firmware waits for the rising edge of the zero-crossing pulse and then delays the coil command by

```
wait_on  = t_on  - t_corr_on      (t_corr_on  = 100 µs)
wait_off = t_off + t_corr_off     (t_corr_off = 200 µs)
```

`t_on` / `t_off` are the measured operate and release times. The correction constants combine all systematic delays of the detection chain (lead of the pulse edge, optocoupler, TOSLINK, polling loop). They were calibrated with an oscilloscope using the firmware's *ZC oscilloscope* mode, until contact closing and opening lay exactly on the zero crossing.

How accurate does it need to be? Near the zero crossing the mains voltage rises by about 100 V per ms (325 V · 2π · 50 Hz). A timing error of 50 µs therefore means about 5 V at the contact, and 100 µs means about 10 V. AgNi contacts start to arc at about 14 V. The total of interrupt latency, timer resolution and execution time should stay well below 100 µs.

## Minimal circuit

| Function | Part | Note |
|---|---|---|
| Optocoupler | H11AA1 | AC input (antiparallel LEDs), so no bridge rectifier is needed |
| R1, R2 | 2 × 100 kΩ, 0.25 W | in series: splits the voltage stress, ~0.13 W each |
| R3 | 10 kΩ | pull-up to 3.3 V. With an internal pull-up the edge is slower, so recalibrate |
| R4 | 1 kΩ | base resistor |
| Driver | NPN, e.g. BC337 | sized for the coil current |
| D2 | 1N4148 | flyback diode, mandatory |

Connected between L and N, the detector works regardless of how the plug is inserted. The mains side needs the same IPC-2221 clearances as any other mains circuit (≥ 2.5 mm for 301–500 V peak, external conductors).

## Firmware

Requirements are minimal: one interrupt input, one output, and a timer with ≤ 1 µs resolution. Any 8-bit MCU is enough. Trigger the coil from a **timer compare**, not a busy-wait, as soon as the MCU has other work to do.

The example in [`examples/zero_cross_minimal`](../examples/zero_cross_minimal/zero_cross_minimal.ino) implements this on an ESP32:

1. `RelaySet(on)` only stores the request.
2. The ZC edge interrupt starts a one-shot timer with the wait time.
3. The timer callback drives the coil.
4. If no ZC edge arrives within 100 ms, the relay is switched directly, so a broken detector never blocks the device.

Things to keep in mind:

- Recalibrate the correction constants for your own circuit and for 60 Hz grids.
- The method synchronises to the **voltage** zero crossing. That is correct for resistive loads. For inductive loads, current and voltage zero crossings differ, which was not part of this study.
- The release time increases with wear. A fixed switch-off delay loses accuracy towards the end of life.
