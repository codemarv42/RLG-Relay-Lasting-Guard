// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// GPIO assignment of the RLG master board (rev. 2, ESP32-D0WDQ6).

#ifndef RLG_PINS_H
#define RLG_PINS_H

// Status LED (active low, red + green = yellow)
#define PIN_LED_RED        2
#define PIN_LED_GREEN      33

// Relay outputs
// MAIN_RELAY, RELAY1 and RELAY2 drive the TOSLINK transmitters to the slave board.
// RELAY3 drives the reference socket on the master board (switch-time measurement).
#define PIN_MAIN_RELAY     13
#define PIN_RELAY1         16
#define PIN_RELAY2         17
#define PIN_RELAY3         32

// SPI / micro SD card
#define PIN_SPI_SCK        18
#define PIN_SPI_MISO       19
#define PIN_SPI_MOSI       23
#define PIN_SD_CS          5
#define PIN_SD_DETECT      36   // input only, external pull-up; LOW = card inserted

// Rotary encoder
#define PIN_ENC_SW         12
#define PIN_ENC_A          14
#define PIN_ENC_B          15

// Sensors
#define PIN_ONEWIRE        4    // DS18B20 temperature sensors
#define PIN_ZC_MAIN_SLAVE  25   // voltage behind the safety relay (via TOSLINK)
#define PIN_ZC_R1_SLAVE    26   // voltage behind test relay 1 (via TOSLINK)
#define PIN_ZC_R2_SLAVE    27   // voltage behind test relay 2 (via TOSLINK)
#define PIN_ZC_PE          35   // L-PE detection (reversed-polarity check)
#define PIN_ZC_MAINS       39   // L-N zero-crossing reference
#define PIN_RELAY3_CONTACT 34   // contact state of the reference socket

// I2C
#define OLED_I2C_ADDRESS   0x3C

#endif
