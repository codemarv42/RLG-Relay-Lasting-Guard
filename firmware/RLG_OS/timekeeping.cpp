// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include "timekeeping.h"

#include <esp_timer.h>

uint64_t UptimeMs() {
  return (uint64_t)esp_timer_get_time() / 1000ULL;
}

// ============================ Relay timers ============================

static uint64_t timerStartMs[2]   = {0, 0};
static uint64_t timerElapsedMs[2] = {0, 0};
static bool     timerRunning[2]   = {false, false};

void RelayTimerStart(int idx) {
  timerElapsedMs[idx] = 0;
  RelayTimerResume(idx);
}

void RelayTimerResume(int idx) {
  timerStartMs[idx] = UptimeMs() - timerElapsedMs[idx];
  timerRunning[idx] = true;
}

void RelayTimerStop(int idx) {
  if (!timerRunning[idx]) return;
  timerElapsedMs[idx] = UptimeMs() - timerStartMs[idx];
  timerRunning[idx]   = false;
}

void RelayTimerSet(int idx, uint64_t ms) {
  timerElapsedMs[idx] = ms;
  if (timerRunning[idx]) timerStartMs[idx] = UptimeMs() - ms;
}

uint64_t RelayTimerElapsedMs(int idx) {
  return timerRunning[idx] ? (UptimeMs() - timerStartMs[idx]) : timerElapsedMs[idx];
}

bool RelayTimerRunning(int idx) {
  return timerRunning[idx];
}

// ============================= Wall clock =============================

static bool     clockSet       = false;
static int64_t  clockBaseEpoch = 0;   // seconds since 1970-01-01 at clockBaseUptime
static uint64_t clockBaseUptime = 0;  // ms

// Howard Hinnant's days_from_civil / civil_from_days (proleptic Gregorian calendar)
static int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

static void civilFromDays(int64_t z, int& y, int& m, int& d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  d = (int)(doy - (153 * mp + 2) / 5 + 1);
  m = (int)(mp < 10 ? mp + 3 : mp - 9);
  y = (int)(yoe + era * 400 + (m <= 2));
}

void ClockSet(int hour, int minute, int second, int day, int month, int year) {
  clockBaseEpoch  = daysFromCivil(year, month, day) * 86400LL + hour * 3600LL + minute * 60LL + second;
  clockBaseUptime = UptimeMs();
  clockSet        = true;
}

bool ClockIsSet() {
  return clockSet;
}

static void clockBreakdown(int& y, int& mo, int& d, int& h, int& mi, int& s) {
  const int64_t now  = clockBaseEpoch + (int64_t)((UptimeMs() - clockBaseUptime) / 1000ULL);
  const int64_t days = now / 86400;
  int64_t rest = now % 86400;
  civilFromDays(days, y, mo, d);
  h  = (int)(rest / 3600); rest %= 3600;
  mi = (int)(rest / 60);
  s  = (int)(rest % 60);
}

String ClockNow() {
  if (!clockSet) return String("--/--/---- --:--:--");
  int y, mo, d, h, mi, s;
  clockBreakdown(y, mo, d, h, mi, s);
  char buf[24];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d", d, mo, y, h, mi, s);
  return String(buf);
}
