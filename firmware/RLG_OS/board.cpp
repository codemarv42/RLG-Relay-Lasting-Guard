// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include <Arduino.h>

#include "board.h"
#include "config.h"
#include "pins.h"

void BoardInitPins() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_MAIN_RELAY, OUTPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_RELAY3, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);

  pinMode(PIN_ENC_SW, INPUT);
  pinMode(PIN_ZC_MAIN_SLAVE, INPUT);
  pinMode(PIN_ZC_R1_SLAVE, INPUT);
  pinMode(PIN_ZC_R2_SLAVE, INPUT);
  pinMode(PIN_ZC_PE, INPUT);
  pinMode(PIN_ZC_MAINS, INPUT);
  pinMode(PIN_RELAY3_CONTACT, INPUT);
  pinMode(PIN_SD_DETECT, INPUT);

  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);

  StopAllRelays();
  SetRelayOutput(PIN_RELAY3, false);
}

void SetLed(LedColor color) {
  // LEDs are active low
  digitalWrite(PIN_LED_RED, (color == LED_RED || color == LED_YELLOW) ? LOW : HIGH);
  digitalWrite(PIN_LED_GREEN, (color == LED_GREEN || color == LED_YELLOW) ? LOW : HIGH);
}

void SetRelayOutput(int pin, bool on) {
  if (pin == PIN_RELAY3) {
    digitalWrite(PIN_RELAY3, on ? LOW : HIGH);
  } else {
    analogWrite(pin, on ? TOSLINK_PWM_ON : TOSLINK_PWM_OFF);
  }
}

void StopAllRelays() {
  SetRelayOutput(PIN_RELAY1, false);
  SetRelayOutput(PIN_RELAY2, false);
  SetRelayOutput(PIN_MAIN_RELAY, false);
}
