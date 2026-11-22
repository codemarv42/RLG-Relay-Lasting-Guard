// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Serial user interface (115200 baud). Commands are entered as numbers + Enter.

#ifndef RLG_SERIAL_UI_H
#define RLG_SERIAL_UI_H

void SerialMenuHandle();     // processes one pending command, if any
void SerialMenuPrint();      // full status screen with command list
void SerialPrintLiveData();  // compact status output (menu disabled)
void SerialClearScreen();

// Measures the switch time in the reference socket and stores it for channel idx
// (idx < 0: measure only). Shared by the serial menu and future UI paths.
bool MeasureAndStoreSwitchTime(int idx);

#endif
