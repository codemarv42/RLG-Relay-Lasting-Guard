// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Mains detection, frequency, temperature and relay switch-time measurement.

#ifndef RLG_MEASURE_H
#define RLG_MEASURE_H

#include <stdint.h>

// ---- Mains detection ----
// Counts edges of a zero-crossing signal (one pulse per half-wave).
float MeasureFrequency(int pin, unsigned long windowMs);

// A channel is live if it shows a plausible mains frequency (40..70 Hz)
// after waiting settleMs. Much more robust than reading a static level.
bool IsLineLive(int pin, unsigned int settleMs, unsigned long windowMs);

// True if the pin is pulled low at least once within 100 ms (L-N / L-PE present).
bool SignalPresent(int pin);

extern float MainsFrequency;

// ---- Temperature (DS18B20 on OneWire) ----
enum TempSensor { TEMP_LOAD1 = 0, TEMP_LOAD2 = 1, TEMP_AUX = 2 };
bool TemperatureInit();
void TemperatureUpdate();
extern float TempC[3];
extern int   TempSensorCount;

// ---- Switch time of the relay in the reference socket (RELAY3) ----
// Averages SWITCH_TIME_SAMPLES operate/release cycles. Returns false on timeout
// (no relay inserted or contact not moving); outputs are only written on success.
bool MeasureSwitchTime(uint32_t& onUs, uint32_t& offUs);

#endif
