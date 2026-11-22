// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard
// Compile-time configuration. Values marked [thesis] were used for all valid
// measurements; changing them makes new results incomparable to the published data.

#ifndef RLG_CONFIG_H
#define RLG_CONFIG_H

#include <stdint.h>

// ============================== Firmware ==============================
#define RLG_VERSION              "1.2.0"
#define SERIAL_BAUD              115200
#define SERIAL_MENU_ENABLED      1        // 1 = interactive menu, 0 = plain live data

// ============================ Relay timing ============================
#define RELAY_ON_TIME_MS         80       // [thesis] contact closed time per cycle
#define COOLDOWN_MIN_MS          800      // [thesis] random pause before each ON command
#define COOLDOWN_MAX_MS          820      // [thesis]

// Default switch times (used until the relay was measured in the reference socket)
#define DEFAULT_SWITCH_TIME_ON_US   4500
#define DEFAULT_SWITCH_TIME_OFF_US  4000

// Correction constants of the zero-crossing chain (edge lead, optocoupler,
// TOSLINK and polling latency). Determined with an oscilloscope, see docs.
//   wait_on  = t_on  - ZC_CORRECTION_ON_US
//   wait_off = t_off + ZC_CORRECTION_OFF_US
#define ZC_CORRECTION_ON_US      100      // [thesis]
#define ZC_CORRECTION_OFF_US     200      // [thesis]
#define ZC_TIMEOUT_US            100000UL // fall back to direct switching after 100 ms

// Switch-time measurement in the reference socket
#define SWITCH_TIME_SAMPLES      20
#define SWITCH_TIME_TIMEOUT_US   50000UL  // abort if the contact does not move within 50 ms
#define SWITCH_TIME_PAUSE_ON_MS  100
#define SWITCH_TIME_PAUSE_OFF_MS 500

// TOSLINK receivers need a toggling signal, so ON/OFF are sent as PWM with
// almost full / almost empty duty cycle.
#define TOSLINK_PWM_ON           254
#define TOSLINK_PWM_OFF          1

// ======================= Line / failure detection =====================
#define LINE_FREQ_MIN_HZ         40       // a channel counts as live between 40 and 70 Hz
#define LINE_FREQ_MAX_HZ         70
#define MAX_RELAY_FAILS          40       // [thesis] failure criterion

// ============================= Protection =============================
#define LOAD_MAX_TEMP_C          60       // pause test above this load temperature
#define LOAD_RESUME_TEMP_C       40       // resume below this temperature
#define MIN_TEMP_SENSORS         2

// DallasTemperature enumerates sensors by ROM address, so the index -> position
// mapping depends on the actual sensors. Adjust to your setup.
#define TEMP_IDX_LOAD1           1
#define TEMP_IDX_LOAD2           0
#define TEMP_IDX_AUX             2

// ============================== Logging ===============================
#define SD_LOG_INTERVAL_S        60       // log row + state backup interval
#define LOG_DIR                  "/RLG_LOGS"

// ============================= Simulation =============================
#define SIMULATION_MODE          0        // 1 = no real switching (UI / SD testing)
#define SIM_STEP_INTERVAL_S      10

// =========================== Operating modes ==========================
enum OperatingModeId : int {
  MODE_LIFESPAN    = 0,   // continuous lifetime test
  MODE_CONTACT_RES = 1,   // pauses at fixed cycle counts for contact resistance measurement
};

// Pause points for MODE_CONTACT_RES; afterwards every CONTACT_RES_STEP cycles
static const uint64_t CONTACT_RES_STOPS[] = {5000, 10000, 15000, 25000, 50000, 100000, 150000};
static const int      CONTACT_RES_STOPS_COUNT = sizeof(CONTACT_RES_STOPS) / sizeof(CONTACT_RES_STOPS[0]);
static const uint64_t CONTACT_RES_STEP = 50000;

#endif
