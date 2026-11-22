// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Lifetime test: test channels, zero-crossing switching, safety test and the
// FreeRTOS task that runs the switching cycles on core 0.

#ifndef RLG_RELAY_TEST_H
#define RLG_RELAY_TEST_H

#include <stdint.h>

// One test channel = one relay socket on the slave board.
struct TestChannel {
  // fixed hardware mapping
  const char* tag;          // "R1" / "R2", used in file names and UI
  int     outputPin;        // TOSLINK transmitter
  int     sensePin;         // voltage behind the relay (TOSLINK receiver)
  uint8_t errNotClosing;
  uint8_t errWelded;
  uint8_t errDoneExists;
  uint8_t errIdUnset;

  // configuration (persisted in the settings file)
  uint32_t id;                  // relay number of the specimen, 0 = unset
  bool     enabled;
  bool     zeroCross;           // true = zero-crossing switching ("protection")
  uint32_t switchTimeOnUs;      // measured operate time
  uint32_t switchTimeOffUs;     // measured release time
  bool     switchTimeMeasured;

  // test state (persisted in the state file)
  uint64_t switches;
  uint32_t fails;
};

constexpr int CHANNEL_COUNT = 2;
extern TestChannel Channels[CHANNEL_COUNT];

extern volatile bool TestRunning;
extern volatile bool StopRequested;          // set by the test task, handled in loop()
extern volatile bool WaitingForContactMeasurement;
extern bool          ContactResNeeded[CHANNEL_COUNT];
extern int           OperatingMode;
extern bool          OverTempAlert;
extern bool          FirstTestStart;

// Switching primitives
void SwitchAtZeroCrossing(const TestChannel& ch, bool on);
bool RelaySafetyTest();

// Test control (call from loop() / core 1 only)
void StartRelayTest();
void StopRelayTest();
void PauseForContactMeasurement();
void StopTestTask();
void CheckTemperature();

// Resets counters, timer and switch times of a channel (log files are kept).
void ResetChannel(int idx);

#endif
