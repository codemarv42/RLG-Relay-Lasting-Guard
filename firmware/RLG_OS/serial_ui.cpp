// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include <Arduino.h>
#include <stdarg.h>

#include "board.h"
#include "config.h"
#include "display.h"
#include "error_codes.h"
#include "measure.h"
#include "relay_test.h"
#include "serial_ui.h"
#include "storage.h"
#include "timekeeping.h"

static constexpr int SCREEN_LINES = 48;
static constexpr int MENU_WIDTH   = 40;   // inner width of the menu box

// ================================ Input =================================

static String readLine() {
  while (!Serial.available()) delay(10);
  String s = Serial.readStringUntil('\n');
  s.trim();
  return s;
}

static int readInt() {
  return readLine().toInt();
}

static unsigned long readHex() {
  for (;;) {
    String s = readLine();
    if (s.startsWith("0x") || s.startsWith("0X")) return strtoul(s.c_str() + 2, NULL, 16);
    Serial.println(F("Invalid. Format: 0x1A"));
  }
}

// Asks "relay 1 or 2" and returns the channel index, or -1.
static int readChannel(const __FlashStringHelper* prompt) {
  Serial.println(prompt);
  const int r = readInt();
  if (r < 1 || r > CHANNEL_COUNT) {
    Serial.println(F("Invalid relay."));
    return -1;
  }
  return r - 1;
}

static bool confirmWithoutClock() {
  if (ClockIsSet()) return true;
  Serial.println(F("WARNING: No clock set, timestamps will be missing. Continue? (y/n)"));
  const unsigned long t0 = millis();
  while (millis() - t0 < 10000) {
    if (Serial.available()) {
      String a = Serial.readStringUntil('\n');
      a.trim();
      return a == "y" || a == "Y";
    }
    delay(100);
  }
  return false;
}

// =============================== Commands ===============================

bool MeasureAndStoreSwitchTime(int idx) {
  DrawSwitchTimeMeasuring(idx);
  uint32_t onUs = 0, offUs = 0;
  if (!MeasureSwitchTime(onUs, offUs)) {
    ErrorCode = Err::SWITCH_TIME_TIMEOUT;
    Serial.println(F("Measurement failed: no contact movement (relay inserted?)."));
    return false;
  }
  if (idx >= 0) {
    TestChannel& ch = Channels[idx];
    ch.switchTimeOnUs     = onUs;
    ch.switchTimeOffUs    = offUs;
    ch.switchTimeMeasured = true;
    SaveSettings();
  }
  Serial.printf("Switch time ON: %lu us, OFF: %lu us%s\n", (unsigned long)onUs, (unsigned long)offUs,
                idx >= 0 ? " (saved)" : " (not saved)");
  DrawSwitchTimeResult(onUs, offUs, idx >= 0);
  return true;
}

static void cmdStartStop() {
  if (TestRunning) {
    StopRelayTest();
    Serial.println(F("Test stopped."));
    return;
  }
  if (!confirmWithoutClock()) {
    Serial.println(F("Aborted."));
    return;
  }
  SetLed(LED_YELLOW);
  StartRelayTest();
  delay(1000);
  if (TestRunning) Serial.println(F("Test started."));
  else             Serial.printf("Error 0x%X -> %s\n", (unsigned)ErrorCode, ErrorCodeText(ErrorCode));
}

static void cmdSetRelayId() {
  const int idx = readChannel(F("Relay 1 or 2?"));
  if (idx < 0) return;
  Serial.println(F("Enter relay number as hex (e.g. 0x1A):"));
  const unsigned long id = readHex();
  if (DoneLogExists(idx, id)) {
    Serial.println(F("ERROR: a finished (DONE) log already exists for this number."));
    return;
  }
  Channels[idx].id = id;
  SaveSettings();
  Serial.println(F("Saved."));
}

static void cmdMeasureSwitchTime() {
  Serial.println(F("Switch time measurement (relay in reference socket):"));
  Serial.println(F("  1 = measure -> save to R1"));
  Serial.println(F("  2 = measure -> save to R2"));
  Serial.println(F("  3 = view only"));
  const int opt = readInt();
  if (opt < 1 || opt > 3) return;
  MeasureAndStoreSwitchTime(opt == 3 ? -1 : opt - 1);
  delay(opt == 3 ? 10000 : 5000);
}

static void cmdSafetyTest() {
  if (RelaySafetyTest()) Serial.println(F("Safety test OK."));
  else Serial.printf("Error 0x%X -> %s\n", (unsigned)ErrorCode, ErrorCodeText(ErrorCode));
}

static void cmdLiveData() {
  while (!Serial.available()) {
    TemperatureUpdate();
    SerialPrintLiveData();
    delay(1000);
  }
  Serial.readStringUntil('\n');
}

static void cmdResetChannel() {
  const int idx = readChannel(F("Reset data of relay 1 or 2?"));
  if (idx < 0) return;
  ResetChannel(idx);
  SaveState();
  SaveSettings();
  Serial.println(F("Channel reset (ID = 0x0). Log files on the SD card are kept."));
}

static void cmdToggleEnabled() {
  const int idx = readChannel(F("Enable/disable relay 1 or 2?"));
  if (idx < 0) return;
  Channels[idx].enabled = !Channels[idx].enabled;
  SaveSettings();
  Serial.printf("%s: %s\n", Channels[idx].tag, Channels[idx].enabled ? "ON" : "OFF");
}

static void cmdToggleZeroCross() {
  const int idx = readChannel(F("Toggle zero-crossing switching for relay 1 or 2?"));
  if (idx < 0) return;
  Channels[idx].zeroCross = !Channels[idx].zeroCross;
  SaveSettings();
  Serial.printf("%s zero-crossing: %s\n", Channels[idx].tag, Channels[idx].zeroCross ? "ON" : "OFF");
}

static void cmdSetClock() {
  Serial.println(F("Format: DD.MM.YYYY HH:MM:SS"));
  const String dt = readLine();
  if (dt.length() < 19) {
    Serial.println(F("Invalid format."));
    return;
  }
  ClockSet(dt.substring(11, 13).toInt(), dt.substring(14, 16).toInt(), dt.substring(17, 19).toInt(),
           dt.substring(0, 2).toInt(), dt.substring(3, 5).toInt(), dt.substring(6, 10).toInt());
  Serial.printf("Clock set: %s\n", ClockNow().c_str());
}

void SerialMenuHandle() {
  if (!Serial.available()) return;
  const int cmd = readInt();

  switch (cmd) {
    case 1:  cmdStartStop();         delay(2000); break;
    case 2:  cmdSetRelayId();        delay(2000); break;
    case 3:  cmdMeasureSwitchTime();              break;
    case 4:  cmdSafetyTest();        delay(3000); break;
    case 5:  cmdLiveData();                       break;
    case 6:  SaveState(); Serial.println(F("State saved to SD.")); delay(2000); break;
    case 7:  LoadState(); Serial.println(F("State loaded from SD.")); delay(2000); break;
    case 8:  cmdResetChannel();      delay(2000); break;
    case 9:  cmdToggleEnabled();     delay(2000); break;
    case 10: cmdToggleZeroCross();   delay(2000); break;
    case 11: cmdSetClock();          delay(2000); break;
    case 12: Serial.println(F("Restarting...")); delay(1000); ESP.restart(); break;
    default: break;
  }
}

// ================================ Output ================================

void SerialClearScreen() {
  for (int i = 0; i < SCREEN_LINES; i++) Serial.println();
}

// Prints one box line; content is truncated/padded to MENU_WIDTH - 2 characters.
static void boxLine(const char* fmt, ...) {
  char buf[MENU_WIDTH - 1];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.printf("| %-*s |\n", MENU_WIDTH - 2, buf);
}

static void boxRule() {
  Serial.print('+');
  for (int i = 0; i < MENU_WIDTH; i++) Serial.print('-');
  Serial.println('+');
}

void SerialMenuPrint() {
  SerialClearScreen();
  boxRule();
  boxLine("RLG - Relay Lasting Guard  v%s", RLG_VERSION);
  boxRule();
  boxLine(" 1  %s", TestRunning ? "Stop test" : "Start test");
  boxLine(" 2  Set relay number");
  boxLine(" 3  Measure switch time");
  boxLine(" 4  Safety test");
  boxLine(" 5  Live data monitor");
  boxLine(" 6  Save state to SD");
  boxLine(" 7  Load state from SD");
  boxLine(" 8  Reset relay channel");
  boxLine(" 9  Enable / disable relay");
  boxLine("10  Toggle zero-crossing switching");
  boxLine("11  Set date & time");
  boxLine("12  Restart");
  boxRule();
  boxLine("Mode   : %s", OperatingMode == MODE_LIFESPAN ? "Lifespan" : "Contact resistance");
  boxLine("Status : %s", TestRunning ? "RUNNING" : "IDLE");
  boxLine("Clock  : %s", ClockIsSet() ? ClockNow().c_str() : "NOT SET");
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    const TestChannel& ch = Channels[i];
    boxRule();
    boxLine("%s [0x%02lX]  %s  ZC:%s", ch.tag, (unsigned long)ch.id, ch.enabled ? "ON " : "OFF", ch.zeroCross ? "Y" : "N");
    boxLine("  Cycles: %-10lu t: %lus", (unsigned long)ch.switches, (unsigned long)(RelayTimerElapsedMs(i) / 1000ULL));
    boxLine("  Fails: %lu/%d  ON:%luus OFF:%luus", (unsigned long)ch.fails, MAX_RELAY_FAILS,
            (unsigned long)ch.switchTimeOnUs, (unsigned long)ch.switchTimeOffUs);
    boxLine("  Switch time measured: %s", ch.switchTimeMeasured ? "YES" : "NO");
  }
  boxRule();
  boxLine("Err : 0x%02X %s", (unsigned)ErrorCode, ErrorCodeText(ErrorCode));
  boxLine("Temp: L1=%.1fC L2=%.1fC Aux=%.1fC", TempC[TEMP_LOAD1], TempC[TEMP_LOAD2], TempC[TEMP_AUX]);
  boxLine("Freq: %.0f Hz", MainsFrequency);
  boxRule();
}

void SerialPrintLiveData() {
  SerialClearScreen();
  Serial.println(F("=== RLG live data ==="));
  Serial.printf("Clock    : %s\n", ClockIsSet() ? ClockNow().c_str() : "not set");
  Serial.printf("Error    : 0x%X -> %s\n", (unsigned)ErrorCode, ErrorCodeText(ErrorCode));
  Serial.printf("Mode     : %s\n", OperatingMode == MODE_LIFESPAN ? "Lifespan" : "Contact resistance");
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    const TestChannel& ch = Channels[i];
    Serial.printf("%s       : %lu cycles, %lu/%d fails, ON %lu us, OFF %lu us\n", ch.tag,
                  (unsigned long)ch.switches, (unsigned long)ch.fails, MAX_RELAY_FAILS,
                  (unsigned long)ch.switchTimeOnUs, (unsigned long)ch.switchTimeOffUs);
  }
  Serial.printf("Load 1   : %.1f C\n", TempC[TEMP_LOAD1]);
  Serial.printf("Load 2   : %.1f C\n", TempC[TEMP_LOAD2]);
  Serial.printf("AC freq  : %d Hz\n", (int)MainsFrequency);
}
