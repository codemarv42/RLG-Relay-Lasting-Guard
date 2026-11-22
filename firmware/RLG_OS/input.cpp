// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include <Arduino.h>
#include <ESP32Encoder.h>  // tested with 0.10.2

#include "input.h"
#include "pins.h"

static ESP32Encoder encoder;
static volatile bool buttonFlag = false;
static long lastEncPos = 0;

long EncPosition = 0;
int  EncMaxPos   = 2;
bool EncChanged  = false;

static void IRAM_ATTR onButton() {
  buttonFlag = true;
}

void InputInit() {
  encoder.attachHalfQuad(PIN_ENC_B, PIN_ENC_A);
  encoder.setCount(0);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), onButton, FALLING);
  buttonFlag = false;
}

void EncoderUpdate() {
  long pos = encoder.getCount() / 2;   // half-quad: two counts per detent
  if (pos > EncMaxPos) { encoder.setCount(EncMaxPos * 2); pos = EncMaxPos; }
  if (pos < 0)         { encoder.setCount(0);             pos = 0; }
  EncPosition = pos;
  if (pos != lastEncPos) {
    EncChanged = true;
    lastEncPos = pos;
  }
}

void EncoderReset() {
  encoder.setCount(0);
  EncPosition = 0;
  lastEncPos  = 0;
  EncChanged  = false;
}

void EncoderSet(int value) {
  encoder.setCount(value * 2);
  EncPosition = value;
  lastEncPos  = value;
}

bool ButtonPressed() {
  if (!buttonFlag) return false;
  buttonFlag = false;
  return true;
}

void ButtonClear() {
  buttonFlag = false;
}
