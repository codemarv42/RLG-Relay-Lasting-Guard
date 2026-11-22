// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// 64-bit uptime, per-channel test timers and a user-set wall clock
// (the board has no RTC, date/time is entered at start-up).

#ifndef RLG_TIMEKEEPING_H
#define RLG_TIMEKEEPING_H

#include <Arduino.h>
#include <stdint.h>

// Milliseconds since boot, does not overflow after 49 days like millis().
uint64_t UptimeMs();

// ---- Test duration per channel (0 = relay 1, 1 = relay 2) ----
void     RelayTimerStart(int idx);              // start from zero
void     RelayTimerResume(int idx);             // continue from stored elapsed time
void     RelayTimerStop(int idx);               // no-op if not running
void     RelayTimerSet(int idx, uint64_t ms);   // restore from SD (timer must be stopped)
uint64_t RelayTimerElapsedMs(int idx);
bool     RelayTimerRunning(int idx);

// ---- Wall clock ----
void   ClockSet(int hour, int minute, int second, int day, int month, int year);
bool   ClockIsSet();
String ClockNow();          // "DD/MM/YYYY HH:MM:SS" or "--/--/---- --:--:--" if not set

#endif
