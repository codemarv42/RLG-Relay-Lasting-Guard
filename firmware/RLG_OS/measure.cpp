// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include <Arduino.h>
#include <DallasTemperature.h>  // tested with 3.9.0
#include <OneWire.h>            // tested with 2.3.7

#include "board.h"
#include "config.h"
#include "measure.h"
#include "pins.h"

static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature sensors(&oneWire);

float MainsFrequency  = 0.0f;
float TempC[3]        = {0.0f, 0.0f, 0.0f};
int   TempSensorCount = 0;

// ============================ Mains detection ===========================

// The ZC signal pulses once per half-wave, i.e. 4 edges per mains period.
float MeasureFrequency(int pin, unsigned long windowMs) {
  int prev = digitalRead(pin);
  int edges = 0;
  unsigned long start = millis();
  delay(2);
  while (millis() - start < windowMs) {
    int curr = digitalRead(pin);
    if (curr != prev) {
      edges++;
      prev = curr;
    }
  }
  if (edges < 2) return 0.0f;
  return (float)edges / (windowMs / 1000.0f) / 4.0f;
}

bool IsLineLive(int pin, unsigned int settleMs, unsigned long windowMs) {
  delay(settleMs);
  int f = (int)MeasureFrequency(pin, windowMs);
  return (f >= LINE_FREQ_MIN_HZ && f <= LINE_FREQ_MAX_HZ);
}

bool SignalPresent(int pin) {
  delay(30);
  unsigned long start = millis();
  while (millis() - start < 100) {
    if (digitalRead(pin) == LOW) return true;
  }
  return false;
}

// ============================== Temperature =============================

bool TemperatureInit() {
  sensors.begin();
  TempSensorCount = sensors.getDeviceCount();
  sensors.setResolution(12);
  return TempSensorCount >= MIN_TEMP_SENSORS;
}

static float readSensor(int index) {
  float t = sensors.getTempCByIndex(index);
  return (t < -40.0f) ? 0.0f : t;   // DEVICE_DISCONNECTED_C = -127
}

void TemperatureUpdate() {
  sensors.requestTemperatures();
  TempC[TEMP_LOAD1] = readSensor(TEMP_IDX_LOAD1);
  TempC[TEMP_LOAD2] = readSensor(TEMP_IDX_LOAD2);
  TempC[TEMP_AUX]   = readSensor(TEMP_IDX_AUX);
}

// ========================== Switch time (RELAY3) ========================

// Busy-waits until pin == level. The timestamp is taken right after the level
// change is seen, exactly like the original measurement loop.
static bool waitForLevel(int pin, int level, unsigned long t0, uint32_t& elapsedUs) {
  for (;;) {
    if (digitalRead(pin) == level) {
      elapsedUs = (uint32_t)(micros() - t0);
      return true;
    }
    if (micros() - t0 > SWITCH_TIME_TIMEOUT_US) return false;
  }
}

bool MeasureSwitchTime(uint32_t& onUs, uint32_t& offUs) {
  uint64_t sumOn = 0, sumOff = 0;

  SetRelayOutput(PIN_RELAY3, false);
  delay(2);

  for (int i = 0; i < SWITCH_TIME_SAMPLES; i++) {
    uint32_t t;

    unsigned long t0 = micros();
    SetRelayOutput(PIN_RELAY3, true);
    if (!waitForLevel(PIN_RELAY3_CONTACT, LOW, t0, t)) {   // first contact closure
      SetRelayOutput(PIN_RELAY3, false);
      return false;
    }
    sumOn += t;
    delay(SWITCH_TIME_PAUSE_ON_MS);

    t0 = micros();
    SetRelayOutput(PIN_RELAY3, false);
    if (!waitForLevel(PIN_RELAY3_CONTACT, HIGH, t0, t)) {  // first contact opening
      return false;
    }
    sumOff += t;
    delay(SWITCH_TIME_PAUSE_OFF_MS);
  }

  onUs  = (uint32_t)(sumOn / SWITCH_TIME_SAMPLES);
  offUs = (uint32_t)(sumOff / SWITCH_TIME_SAMPLES);
  return true;
}
