// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// =====================================================================
//  RLG OS - Relay Lasting Guard
//  Firmware of the automated relay lifetime test bench.
//  Author: Marvin Ramm
//  https://github.com/codemarv42/RLG-Relay-Lasting-Guard
//
//  Core 0: relay test task (timing critical, see relay_test.cpp)
//  Core 1: Arduino loop - UI, temperature, SD logging, mains frequency
// =====================================================================

#include "board.h"
#include "config.h"
#include "display.h"
#include "error_codes.h"
#include "input.h"
#include "measure.h"
#include "pins.h"
#include "relay_test.h"
#include "serial_ui.h"
#include "storage.h"
#include "timekeeping.h"

static uint64_t lastSlowTick    = 0;   // temperature + serial (2 s)
static uint64_t lastDisplayTick = 0;   // display (500 ms)
static uint64_t lastLogTick     = 0;   // SD log + state (SD_LOG_INTERVAL_S)
static uint64_t lastFreqTick    = 0;   // mains frequency (10 s)

static volatile bool sdCardChanged = false;
static void IRAM_ATTR onSdCardDetect() { sdCardChanged = true; }

// ============================== Start-up ===============================

// Waits up to timeoutMs for a button press.
static bool waitForButton(unsigned long timeoutMs) {
  ButtonClear();
  const unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (ButtonPressed()) return true;
    delay(50);
  }
  return false;
}

static int selectWithEncoder(int maxPos, void (*draw)(int)) {
  EncMaxPos = maxPos;
  EncoderReset();
  ButtonClear();
  for (;;) {
    EncoderUpdate();
    draw((int)EncPosition);
    if (ButtonPressed()) return (int)EncPosition;
    delay(30);
  }
}

// Oscilloscope mode for calibrating ZC_CORRECTION_*: switches a reference relay in
// socket 1 with 1 s ON / 5 s OFF, either free-running or zero-crossing synchronised
// (using the switch time stored for R1). Press the encoder to leave.
static void runScopeMode() {
  DrawScopeConfirm();
  ButtonClear();
  while (!ButtonPressed()) delay(50);
  delay(500);

  const bool sync = (selectWithEncoder(1, DrawScopeSubMenu) == 1);
  delay(500);
  ButtonClear();

  SetRelayOutput(PIN_MAIN_RELAY, true);
  delay(250);
  for (;;) {
    if (sync) SwitchAtZeroCrossing(Channels[0], true);
    else      SetRelayOutput(Channels[0].outputPin, true);
    SetLed(LED_GREEN);
    if (waitForButton(1000)) break;

    if (sync) SwitchAtZeroCrossing(Channels[0], false);
    else      SetRelayOutput(Channels[0].outputPin, false);
    SetLed(LED_YELLOW);
    if (waitForButton(5000)) break;
  }
  SetRelayOutput(Channels[0].outputPin, false);
  SetRelayOutput(PIN_MAIN_RELAY, false);
  SetLed(LED_OFF);
}

static void selectOperatingMode() {
  Serial.println(F("\n=== Select operating mode ==="));
  Serial.println(F("  0 = Lifespan test"));
  Serial.println(F("  1 = Contact resistance test (with pauses)"));
  Serial.println(F("  2 = ZC oscilloscope (encoder only)"));

  EncMaxPos = 2;
  EncoderReset();
  ButtonClear();
  for (;;) {
    EncoderUpdate();
    DrawModeSelect((int)EncPosition);

    if (ButtonPressed()) {
      delay(300);
      if (EncPosition == 2) {
        runScopeMode();
        EncMaxPos = 2;
        EncoderReset();
        ButtonClear();
        continue;
      }
      OperatingMode = (int)EncPosition;
      break;
    }
    if (Serial.available()) {
      String s = Serial.readStringUntil('\n');
      s.trim();
      const int v = s.toInt();
      if (s.length() > 0 && v >= 0 && v <= 1) { OperatingMode = v; break; }
    }
    delay(40);
  }
  Serial.printf("Mode: %s\n", OperatingMode == MODE_LIFESPAN ? "Lifespan" : "Contact resistance");
}

static void askForClock() {
  Serial.println(F("\nSet date & time? Encoder button = yes, wait 5 s = no (or serial cmd 11 later)"));
  DrawAskClock();
  if (!waitForButton(5000)) return;
  delay(500);

  static const int minV[6] = {0, 0, 0, 1, 1, 2020};
  static const int maxV[6] = {23, 59, 59, 31, 12, 2099};
  int v[6] = {0, 0, 0, 1, 1, 2026};   // H M S D Mo Y

  for (int field = 0; field < 6; field++) {
    EncMaxPos = maxV[field] - minV[field];
    EncoderSet(v[field] - minV[field]);
    ButtonClear();
    for (;;) {
      EncoderUpdate();
      v[field] = minV[field] + (int)EncPosition;
      DrawSetTime(field, v);
      if (ButtonPressed()) { delay(300); break; }
      delay(30);
    }
  }
  ClockSet(v[0], v[1], v[2], v[3], v[4], v[5]);
  Serial.printf("Clock set: %s\n", ClockNow().c_str());
}

static void askForPreviousState() {
  Serial.println(F("\nLoad previous test state from SD? (y/n, 5 s timeout)"));
  const unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (Serial.available()) {
      String s = Serial.readStringUntil('\n');
      s.trim();
      if ((s == "y" || s == "Y") && LoadState()) Serial.println(F("State loaded."));
      return;
    }
    delay(100);
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  BoardInitPins();
  SetLed(LED_YELLOW);
  delay(2000);

  Serial.println(F("RLG - Relay Lasting Guard, firmware " RLG_VERSION));
  DisplayInit();
  DrawBootScreen();
  delay(2500);

  // GPIOs are undefined during boot, so the slave fuse is switched on only now.
  SerialClearScreen();
  Serial.println(F("Switch on the slave fuse, then press the encoder..."));
  DrawFuseScreen();
  while (digitalRead(PIN_ENC_SW) == HIGH) delay(1);
  delay(500);

  if (!TemperatureInit()) {
    ErrorCode = Err::TEMP_SENSORS;
    SetLed(LED_RED);
    Serial.println(F("WARNING: less than 2 DS18B20 sensors found."));
  }

  StorageInit();
  LoadSettings();
  InputInit();

  MainsFrequency = MeasureFrequency(PIN_ZC_MAINS, 500);
  Serial.printf("Mains frequency: %d Hz\n", (int)MainsFrequency);

  if (ErrorCode == Err::OK) SetLed(LED_GREEN);

  selectOperatingMode();
  EncMaxPos = 3;
  askForClock();
  EncMaxPos = 3;
  EncoderReset();
  askForPreviousState();

  delay(300);
  ButtonClear();   // a press during set-up must not start the test

  // Card detect: LOW = inserted. Pin 36 is input-only, the pull-up is on the PCB.
  attachInterrupt(digitalPinToInterrupt(PIN_SD_DETECT), onSdCardDetect, CHANGE);
}

// ================================ Loop =================================

static void handleButton() {
  if (!ButtonPressed()) return;
  delay(300);

  if (WaitingForContactMeasurement && !TestRunning) {   // resume after a pause
    WaitingForContactMeasurement = false;
    StopRequested = false;
    ContactResNeeded[0] = ContactResNeeded[1] = false;
    delay(300);
    StartRelayTest();
  } else if (TestRunning) {
    StopRelayTest();
    Serial.println(F("Test stopped. Data saved."));
  } else {
    if (!ClockIsSet()) {
      DrawNoClockWarning();
      Serial.println(F("WARNING: no clock set. Press again within 4 s to start anyway."));
      if (!waitForButton(4000)) {
        Serial.println(F("Start cancelled. Use serial cmd 11 to set the clock."));
        return;
      }
    }
    StartRelayTest();
  }
}

static void handleSdCardChange() {
  if (!sdCardChanged) return;
  sdCardChanged = false;
  delay(100);
  if (digitalRead(PIN_SD_DETECT) == HIGH) {
    Serial.println(F("[SD] Card removed."));
    return;
  }
  delay(200);
  if (StorageRemount()) {
    ErrorCode = Err::OK;
    Serial.println(F("[SD] Card inserted and mounted."));
  } else {
    Serial.println(F("[SD] Mount failed after insertion."));
  }
}

void loop() {
  const uint64_t now = UptimeMs();
  EncoderUpdate();

  // Stop / pause requested by the test task (it cannot delete itself safely).
  if (StopRequested && TestRunning) {
    if (WaitingForContactMeasurement) PauseForContactMeasurement();
    else                              StopRelayTest();
  }

  handleButton();

  if (now - lastSlowTick >= 2000) {
    lastSlowTick = now;
    TemperatureUpdate();
    CheckTemperature();
    if (SERIAL_MENU_ENABLED) {
      SerialMenuHandle();
      SerialMenuPrint();
    } else {
      SerialPrintLiveData();
    }
  }

  if (now - lastLogTick >= SD_LOG_INTERVAL_S * 1000ULL) {
    lastLogTick = now;
    WriteLogRows();
    if (TestRunning) SaveState();
  }

  if (now - lastDisplayTick >= 500 || EncChanged) {
    lastDisplayTick = now;
    EncChanged = false;
    if (WaitingForContactMeasurement) DrawContactMeasurementPause();
    else                              DrawStatusPage((int)EncPosition);
  }

  if (now - lastFreqTick >= 10000) {
    lastFreqTick = now;
    MainsFrequency = MeasureFrequency(PIN_ZC_MAINS, 200);
  }

  handleSdCardChange();
}
