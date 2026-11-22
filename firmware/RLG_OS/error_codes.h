// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Error codes. The numeric values are written to the SD logs (TEST_END:0x..),
// so they must stay stable.

#ifndef RLG_ERROR_CODES_H
#define RLG_ERROR_CODES_H

#include <stdint.h>

namespace Err {
constexpr uint8_t OK                  = 0x00;
constexpr uint8_t LN_REVERSED         = 0x01;  // no L-PE signal: L and N swapped
constexpr uint8_t R1_WELDED           = 0x02;  // R1 contact live without activation
constexpr uint8_t R2_WELDED           = 0x03;  // R2 contact live without activation
constexpr uint8_t SLAVE_NO_MAINS      = 0x04;
constexpr uint8_t R1_NOT_CLOSING      = 0x05;
constexpr uint8_t R2_NOT_CLOSING      = 0x06;
constexpr uint8_t TEMP_SENSORS        = 0x07;
constexpr uint8_t MASTER_NO_MAINS     = 0x08;
constexpr uint8_t LOAD1_OVERTEMP      = 0x09;
constexpr uint8_t LOAD2_OVERTEMP      = 0x0A;
constexpr uint8_t MAIN_RELAY_WELDED   = 0x0B;
constexpr uint8_t MAIN_RELAY_NO_CLOSE = 0x0C;
constexpr uint8_t SD_MOUNT            = 0x0D;
constexpr uint8_t SAME_RELAY_ID       = 0x0E;
constexpr uint8_t R1_ID_UNSET         = 0x0F;
constexpr uint8_t R2_ID_UNSET         = 0x10;
constexpr uint8_t SD_IO               = 0x11;
constexpr uint8_t SD_NO_FS            = 0x12;
constexpr uint8_t R1_DONE_EXISTS      = 0x13;
constexpr uint8_t R2_DONE_EXISTS      = 0x14;
constexpr uint8_t SD_NO_CARD          = 0x15;
constexpr uint8_t SWITCH_TIME_TIMEOUT = 0x16;
}  // namespace Err

// Global error state, shared between both cores
extern volatile uint8_t ErrorCode;

const char* ErrorCodeText(uint8_t code);

#endif
