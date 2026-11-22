// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include "error_codes.h"

volatile uint8_t ErrorCode = Err::OK;

const char* ErrorCodeText(uint8_t code) {
  switch (code) {
    case Err::OK:                  return "OK - no error";
    case Err::LN_REVERSED:         return "L and N reversed on master";
    case Err::SLAVE_NO_MAINS:      return "Slave not on mains";
    case Err::MASTER_NO_MAINS:     return "Master not on mains";
    case Err::R1_WELDED:           return "R1: contact live without activation (welded)";
    case Err::R1_NOT_CLOSING:      return "R1: contact did not close";
    case Err::R2_WELDED:           return "R2: contact live without activation (welded)";
    case Err::R2_NOT_CLOSING:      return "R2: contact did not close";
    case Err::MAIN_RELAY_WELDED:   return "Safety relay: contact live without activation";
    case Err::MAIN_RELAY_NO_CLOSE: return "Safety relay: contact did not close";
    case Err::LOAD1_OVERTEMP:      return "Load 1 overtemperature - test paused";
    case Err::LOAD2_OVERTEMP:      return "Load 2 overtemperature - test paused";
    case Err::TEMP_SENSORS:        return "Less than 2 DS18B20 sensors found";
    case Err::SD_MOUNT:            return "SD card mount failed";
    case Err::SD_IO:               return "SD read/write error";
    case Err::SD_NO_FS:            return "SD: unknown or missing filesystem";
    case Err::R1_DONE_EXISTS:      return "SD: DONE log for R1 already exists";
    case Err::R2_DONE_EXISTS:      return "SD: DONE log for R2 already exists";
    case Err::SD_NO_CARD:          return "No SD card inserted";
    case Err::SAME_RELAY_ID:       return "Both relays have the same ID";
    case Err::R1_ID_UNSET:         return "R1 ID is 0x0 (not allowed)";
    case Err::R2_ID_UNSET:         return "R2 ID is 0x0 (not allowed)";
    case Err::SWITCH_TIME_TIMEOUT: return "Switch time: no contact movement";
    default:                       return "Unknown error";
  }
}
