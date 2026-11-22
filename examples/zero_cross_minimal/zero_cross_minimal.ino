// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// =====================================================================
//  Minimal zero-crossing switching for a mechanical relay (ESP32)
//
//  Everything the test bench does beyond this is measurement. To use the
//  method in a product you only need:
//    - a zero-crossing detector (AC-input optocoupler + 2 x 100k + pull-up)
//    - one interrupt-capable input and one output for the coil driver
//    - this code
//
//  Flow:
//    RelaySet(on)       -> only stores the request
//    ZC edge interrupt  -> starts a one-shot timer with the wait time
//    timer callback     -> drives the coil, so the contact moves at the next zero crossing
//    no ZC edge 100 ms  -> fallback: switch directly, never block the device
//
//  !! Mains voltage. The detector is directly connected to the grid, keep the
//  !! IPC-2221 clearances and never work on a live board.
// =====================================================================

#include <Arduino.h>
#include <esp_timer.h>

// ---- Hardware ----
constexpr int PIN_ZC    = 39;   // HIGH while the mains voltage is near zero
constexpr int PIN_RELAY = 16;   // coil driver (NPN / N-MOSFET + flyback diode)

// ---- Timing ----
// Starting point: mean operate / release time of the relay model (APAN3105)
// minus/plus the correction of the detection chain, as in the thesis.
// Calibrate WAIT_ON_US / WAIT_OFF_US for your circuit with an oscilloscope
// (mains voltage + voltage behind the contact) until the contact moves exactly
// at the zero crossing. Re-calibrate for 60 Hz grids.
constexpr uint32_t T_OPERATE_US     = 4732;
constexpr uint32_t T_RELEASE_US     = 4497;
constexpr uint32_t T_CORR_ON_US     = 100;
constexpr uint32_t T_CORR_OFF_US    = 200;
constexpr uint32_t WAIT_ON_US       = T_OPERATE_US - T_CORR_ON_US;
constexpr uint32_t WAIT_OFF_US      = T_RELEASE_US + T_CORR_OFF_US;
constexpr uint32_t ZC_TIMEOUT_MS    = 100;

static esp_timer_handle_t switchTimer;
static volatile bool requestPending = false;
static volatile bool requestedState = false;
static uint32_t requestTimeMs = 0;

// Timer callback: the wait time is over, energise / release the coil now.
static void onSwitchTimer(void*) {
  digitalWrite(PIN_RELAY, requestedState ? HIGH : LOW);
}

// Rising edge of the ZC pulse = reference for the wait time.
static void IRAM_ATTR onZeroCross() {
  if (!requestPending) return;
  requestPending = false;
  esp_timer_start_once(switchTimer, requestedState ? WAIT_ON_US : WAIT_OFF_US);
}

// 1. Called by the application: only remember the request.
void RelaySet(bool on) {
  requestedState = on;
  requestTimeMs  = millis();
  requestPending = true;
}

// Fallback if the ZC signal is missing (call regularly from loop()).
void RelayPoll() {
  if (requestPending && millis() - requestTimeMs > ZC_TIMEOUT_MS) {
    requestPending = false;
    digitalWrite(PIN_RELAY, requestedState ? HIGH : LOW);
  }
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_ZC, INPUT);   // GPIO39 has no internal pull-up, use the external one

  esp_timer_create_args_t args = {};
  args.callback        = &onSwitchTimer;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name            = "relay_zc";
  esp_timer_create(&args, &switchTimer);
  attachInterrupt(digitalPinToInterrupt(PIN_ZC), onZeroCross, RISING);
}

void loop() {
  // Demo: toggle the relay every 2 s
  static uint32_t last = 0;
  static bool state = false;
  if (millis() - last >= 2000) {
    last = millis();
    state = !state;
    RelaySet(state);
  }
  RelayPoll();
}
