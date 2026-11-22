// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Low-level board I/O: GPIO setup, status LED and relay outputs.

#ifndef RLG_BOARD_H
#define RLG_BOARD_H

enum LedColor { LED_OFF, LED_RED, LED_GREEN, LED_YELLOW };

void BoardInitPins();
void SetLed(LedColor color);

// Drives a relay output. Slave relays are driven through TOSLINK (PWM),
// the reference socket RELAY3 on the master board is driven directly (active low).
void SetRelayOutput(int pin, bool on);

// Opens test relays and the safety relay.
void StopAllRelays();

#endif
