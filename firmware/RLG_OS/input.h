// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Rotary encoder with push button.

#ifndef RLG_INPUT_H
#define RLG_INPUT_H

void InputInit();

// Polls the encoder, clamps it to [0, EncMaxPos] and sets EncChanged on movement.
void EncoderUpdate();
void EncoderReset();
void EncoderSet(int value);

// Returns true once per button press (consumes the event).
bool ButtonPressed();
void ButtonClear();

extern long EncPosition;
extern int  EncMaxPos;
extern bool EncChanged;

#endif
