// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// micro SD card: settings, state backup, test logs and firmware update.
//
// Files on the card:
//   /rlg_settings.txt              key=value settings (relay IDs, switch times, ...)
//   /rlg_state.bin                 6 x uint64: elapsed R1/R2, switches R1/R2, fails R1/R2
//   /RLG_LOGS/0x<id>_R<n>_Test.txt running test log (TSV)
//   /RLG_LOGS/DONE_0x<id>_...      finished log, blocks a re-test of the same ID
//   /firmware.bin                  flashed on boot, then renamed to firmware.bak

#ifndef RLG_STORAGE_H
#define RLG_STORAGE_H

#include <stdint.h>

bool StorageInit();      // mount card, apply firmware update, create log folder
bool StorageRemount();   // after card hot-swap

void SaveSettings();
void LoadSettings();

void SaveState();
bool LoadState();

// Appends one TSV row per enabled channel (only while a test is running).
void WriteLogRows();

// Writes the summary block and renames the log to DONE_* (idx: 0 = R1, 1 = R2).
void FinalizeRelayLog(int idx);
bool DoneLogExists(int idx, uint32_t relayId);

#endif
