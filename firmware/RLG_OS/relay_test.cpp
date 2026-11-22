// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
//
// Test cycle per relay (see docs/firmware.md):
//   random cooldown 800..820 ms
//   -> switch ON  (immediately, or synchronised to the zero crossing)
//   -> 80 ms ON   -> check voltage behind the contact (2 confirmations = fail)
//   -> switch OFF (immediately, or synchronised to the zero crossing)
//   -> check that the voltage is gone   (3 confirmations = fail / welded)
//   -> 40 fails end the test for this relay.
//
// The timing of this file is identical to firmware 1.1.4, which was used for
// all valid measurements of the thesis.

#include <Arduino.h>

#include "board.h"
#include "config.h"
#include "error_codes.h"
#include "measure.h"
#include "pins.h"
#include "relay_test.h"
#include "storage.h"
#include "timekeeping.h"

TestChannel Channels[CHANNEL_COUNT] = {
  {"R1", PIN_RELAY1, PIN_ZC_R1_SLAVE, Err::R1_NOT_CLOSING, Err::R1_WELDED, Err::R1_DONE_EXISTS, Err::R1_ID_UNSET,
   0, true, false, DEFAULT_SWITCH_TIME_ON_US, DEFAULT_SWITCH_TIME_OFF_US, false, 0, 0},
  {"R2", PIN_RELAY2, PIN_ZC_R2_SLAVE, Err::R2_NOT_CLOSING, Err::R2_WELDED, Err::R2_DONE_EXISTS, Err::R2_ID_UNSET,
   0, true, false, DEFAULT_SWITCH_TIME_ON_US, DEFAULT_SWITCH_TIME_OFF_US, false, 0, 0},
};

volatile bool TestRunning                  = false;
volatile bool StopRequested                = false;
volatile bool WaitingForContactMeasurement = false;
bool ContactResNeeded[CHANNEL_COUNT]       = {false, false};
int  OperatingMode                         = MODE_LIFESPAN;
bool OverTempAlert                         = false;
bool FirstTestStart                        = true;

static TaskHandle_t testTask = NULL;
static uint64_t lastContactResStop[CHANNEL_COUNT] = {0, 0};

// =========================== Switching primitives ==========================

static void cooldownDelay() {
  delay(COOLDOWN_MIN_MS + (uint32_t)random(COOLDOWN_MAX_MS - COOLDOWN_MIN_MS));
}

void SwitchAtZeroCrossing(const TestChannel& ch, bool on) {
  // Delay between the reference edge and the coil command, see docs/zero-crossing.md
  const uint32_t waitUs = on
      ? (ch.switchTimeOnUs > ZC_CORRECTION_ON_US ? ch.switchTimeOnUs - ZC_CORRECTION_ON_US : 0)
      : (ch.switchTimeOffUs + ZC_CORRECTION_OFF_US);

  // Synchronise to the start of a new ZC pulse regardless of the current level:
  // HIGH -> LOW -> HIGH. Polling keeps the latency in the microsecond range.
  unsigned long t0 = micros();
  while (digitalRead(PIN_ZC_MAINS) != HIGH && (micros() - t0 < ZC_TIMEOUT_US));
  while (digitalRead(PIN_ZC_MAINS) != LOW  && (micros() - t0 < ZC_TIMEOUT_US));
  while (digitalRead(PIN_ZC_MAINS) != HIGH && (micros() - t0 < ZC_TIMEOUT_US));

  // On timeout (no ZC signal) switch directly instead of blocking the test.
  if (micros() - t0 < ZC_TIMEOUT_US) delayMicroseconds(waitUs);
  SetRelayOutput(ch.outputPin, on);
}

static void switchChannel(const TestChannel& ch, bool on) {
  if (on) cooldownDelay();
  if (ch.zeroCross) SwitchAtZeroCrossing(ch, on);
  else              SetRelayOutput(ch.outputPin, on);
}

// =============================== Safety test ===============================

bool RelaySafetyTest() {
  ErrorCode = Err::OK;
  StopAllRelays();
  delay(100);

  if (!SignalPresent(PIN_ZC_MAINS)) { ErrorCode = Err::MASTER_NO_MAINS; return false; }
  if (!SignalPresent(PIN_ZC_PE))    { ErrorCode = Err::LN_REVERSED;     return false; }

  SetRelayOutput(PIN_MAIN_RELAY, true);
  if (!IsLineLive(PIN_ZC_MAIN_SLAVE, 30, 40)) {
    ErrorCode = Err::MAIN_RELAY_NO_CLOSE;
    SetRelayOutput(PIN_MAIN_RELAY, false);
    return false;
  }

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    const TestChannel& ch = Channels[i];
    if (!ch.enabled) continue;
    SetRelayOutput(ch.outputPin, true);
    if (!IsLineLive(ch.sensePin, 30, 40)) { ErrorCode = ch.errNotClosing; StopAllRelays(); return false; }
    SetRelayOutput(ch.outputPin, false);
    if (IsLineLive(ch.sensePin, 30, 40))  { ErrorCode = ch.errWelded;     StopAllRelays(); return false; }
  }

  SetRelayOutput(PIN_MAIN_RELAY, false);
  if (IsLineLive(PIN_ZC_MAIN_SLAVE, 30, 40)) { ErrorCode = Err::MAIN_RELAY_WELDED; return false; }

  return true;
}

// ======================= Contact resistance pauses ========================

static void markPassedContactStops() {
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    lastContactResStop[i] = 0;
    if (!Channels[i].enabled) continue;
    for (int k = 0; k < CONTACT_RES_STOPS_COUNT; k++) {
      if (Channels[i].switches >= CONTACT_RES_STOPS[k]) lastContactResStop[i] = CONTACT_RES_STOPS[k];
    }
  }
}

static void checkContactResStop() {
  if (OperatingMode != MODE_CONTACT_RES) return;

  bool trigger[CHANNEL_COUNT] = {false, false};
  const uint64_t base = CONTACT_RES_STOPS[CONTACT_RES_STOPS_COUNT - 1];

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    const TestChannel& ch = Channels[i];
    if (!ch.enabled) continue;

    for (int k = 0; k < CONTACT_RES_STOPS_COUNT; k++) {
      const uint64_t thr = CONTACT_RES_STOPS[k];
      if (ch.switches >= thr && lastContactResStop[i] != thr) {
        lastContactResStop[i] = thr;
        trigger[i] = true;
      }
    }
    // after the last fixed stop: every CONTACT_RES_STEP cycles
    if (ch.switches > base) {
      const uint64_t next = base + ((ch.switches - base) / CONTACT_RES_STEP) * CONTACT_RES_STEP;
      if (next > base && lastContactResStop[i] != next) {
        lastContactResStop[i] = next;
        trigger[i] = true;
      }
    }
  }

  if (trigger[0] || trigger[1]) {
    ContactResNeeded[0] = trigger[0];
    ContactResNeeded[1] = trigger[1];
    WaitingForContactMeasurement = true;
    StopRequested = true;
    Serial.print(F("[PAUSE] Contact resistance measurement needed:"));
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      if (!trigger[i]) continue;
      Serial.printf(" %s@%lu", Channels[i].tag, (unsigned long)Channels[i].switches);
    }
    Serial.println(F(" cycles. Press encoder to resume."));
  }
}

// ================================ Test task ================================

// Called on core 0 when a relay reached MAX_RELAY_FAILS. The other channel is
// not touched and can be resumed afterwards.
static void handleRelayFailure(int idx, uint8_t err) {
  TestChannel& ch = Channels[idx];
  SetRelayOutput(ch.outputPin, false);
  ErrorCode = err;
  SetLed(LED_RED);

  // Stop both timers so the surviving relay keeps an exact elapsed time.
  RelayTimerStop(0);
  RelayTimerStop(1);

  FinalizeRelayLog(idx);   // writes the summary and renames the log to DONE_*
  ch.enabled = false;      // never test this specimen again
  SaveSettings();
  SaveState();

  StopAllRelays();
  TestRunning   = false;
  StopRequested = true;

  Serial.printf("[FAIL] %s failed (err 0x%X). Log finalized, test stopped.\n", ch.tag, err);
}

// One full ON/OFF cycle. Returns false if the test has to stop.
static bool runCycle(int idx) {
  TestChannel& ch = Channels[idx];
  if (!TestRunning) return false;

  // ---- ON ----
  switchChannel(ch, true);
  delay(RELAY_ON_TIME_MS);

  // Two consecutive failed checks are required, single outliers are ignored.
  if (!IsLineLive(ch.sensePin, 5, 60)) {
    delay(10);
    if (!IsLineLive(ch.sensePin, 5, 60)) ch.fails++;
  }
  if (ch.fails >= MAX_RELAY_FAILS) {
    handleRelayFailure(idx, ch.errNotClosing);
    return false;
  }

  // ---- OFF ----
  switchChannel(ch, false);

  // Voltage still present without activation -> welded contact (3 confirmations).
  if (IsLineLive(ch.sensePin, 10, 60) && IsLineLive(ch.sensePin, 5, 60) && IsLineLive(ch.sensePin, 5, 60)) {
    ch.fails++;
  }
  if (ch.fails >= MAX_RELAY_FAILS) {
    handleRelayFailure(idx, ch.errWelded);
    return false;
  }

  ch.switches++;
  return true;
}

static void runTestRound() {
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (i > 0) vTaskDelay(pdMS_TO_TICKS(100));   // short gap between the relays
    if (!TestRunning || !Channels[i].enabled) continue;
    if (!runCycle(i)) return;
    checkContactResStop();
    if (StopRequested) return;
  }
}

static void testTaskMain(void* /*arg*/) {
#if SIMULATION_MODE
  // Counts cycles and fails without switching, for testing UI / SD / pauses.
  uint64_t lastStep = 0;
  for (;;) {
    if (!StopRequested && UptimeMs() - lastStep >= SIM_STEP_INTERVAL_S * 1000ULL) {
      lastStep = UptimeMs();
      for (int i = 0; i < CHANNEL_COUNT; i++) {
        if (!Channels[i].enabled) continue;
        Channels[i].switches++;
        Channels[i].fails++;
      }
      WriteLogRows();
      checkContactResStop();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
#else
  for (;;) {
    if (StopRequested || WaitingForContactMeasurement || !TestRunning) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }
    runTestRound();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
#endif
}

static void startTestTask() {
  // Pinned to core 0 so display, SD and serial (core 1) cannot disturb the timing.
  xTaskCreatePinnedToCore(testTaskMain, "RelayTest", 10000, NULL, 1, &testTask, 0);
}

void StopTestTask() {
  if (testTask != NULL) {
    vTaskDelete(testTask);
    testTask = NULL;
  }
}

// ============================== Start / stop ===============================

void StartRelayTest() {
  if (TestRunning) return;

  StopTestTask();
  delay(50);

  // Never test a specimen twice under the same number.
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    const TestChannel& ch = Channels[i];
    if (ch.enabled && ch.id != 0 && DoneLogExists(i, ch.id)) {
      ErrorCode = ch.errDoneExists;
      SetLed(LED_RED);
      Serial.printf("ERROR: DONE log already exists for %s. Reset the channel first (cmd 8).\n", ch.tag);
      return;
    }
  }

  if (!RelaySafetyTest()) {
    TestRunning = false;
    SetLed(LED_RED);
    return;
  }

  SetRelayOutput(PIN_MAIN_RELAY, true);
  delay(250);

  StopRequested = false;
  WaitingForContactMeasurement = false;
  TestRunning = true;

  if (OperatingMode == MODE_CONTACT_RES) markPassedContactStops();

  startTestTask();
  SetLed(LED_GREEN);

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (!Channels[i].enabled) continue;
    if (FirstTestStart) RelayTimerStart(i);
    else                RelayTimerResume(i);
  }
  FirstTestStart = false;
}

void StopRelayTest() {
  if (!TestRunning && !StopRequested) return;

  TestRunning = false;
  StopTestTask();
  StopAllRelays();
  RelayTimerStop(0);
  RelayTimerStop(1);

  SaveState();
  SetLed(LED_YELLOW);
  StopRequested = false;
}

// Like StopRelayTest(), but keeps the pause flags so loop() shows the wait screen.
// The log row is written while TestRunning is still true, so the pause is logged.
void PauseForContactMeasurement() {
  StopTestTask();
  StopAllRelays();
  RelayTimerStop(0);
  RelayTimerStop(1);
  SaveState();
  WriteLogRows();
  SetLed(LED_YELLOW);
  TestRunning = false;
}

void CheckTemperature() {
  if (TempC[TEMP_LOAD1] > LOAD_MAX_TEMP_C) {
    ErrorCode = Err::LOAD1_OVERTEMP;
    StopRelayTest();
    OverTempAlert = true;
  } else if (TempC[TEMP_LOAD2] > LOAD_MAX_TEMP_C) {
    ErrorCode = Err::LOAD2_OVERTEMP;
    StopRelayTest();
    OverTempAlert = true;
  } else if (OverTempAlert && TempC[TEMP_LOAD1] < LOAD_RESUME_TEMP_C && TempC[TEMP_LOAD2] < LOAD_RESUME_TEMP_C) {
    OverTempAlert = false;
    ErrorCode = Err::OK;
    StartRelayTest();
  }
}

void ResetChannel(int idx) {
  TestChannel& ch = Channels[idx];
  ch.id                 = 0;
  ch.fails              = 0;
  ch.switches           = 0;
  ch.switchTimeOnUs     = DEFAULT_SWITCH_TIME_ON_US;
  ch.switchTimeOffUs    = DEFAULT_SWITCH_TIME_OFF_US;
  ch.switchTimeMeasured = false;
  RelayTimerStop(idx);
  RelayTimerSet(idx, 0);
}
