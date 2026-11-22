// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// 1.3" SH1106 OLED (128 x 64).

#ifndef RLG_DISPLAY_H
#define RLG_DISPLAY_H

#include <stdint.h>

void DisplayInit();

void DrawBootScreen();
void DrawFuseScreen();
void DrawModeSelect(int selected);
void DrawScopeConfirm();
void DrawScopeSubMenu(int selected);
void DrawAskClock();
void DrawSetTime(int field, const int values[6]);   // H, M, S, D, Mo, Y
void DrawNoClockWarning();
void DrawStatusPage(int page);                       // 4 pages, selected with the encoder
void DrawContactMeasurementPause();
void DrawSwitchTimeMeasuring(int idx);
void DrawSwitchTimeResult(uint32_t onUs, uint32_t offUs, bool saved);

#endif
