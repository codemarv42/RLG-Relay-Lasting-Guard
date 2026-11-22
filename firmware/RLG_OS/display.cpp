// SPDX-License-Identifier: CC-BY-NC-SA-4.0
// Copyright (c) 2024-2026 Marvin Ramm - https://github.com/codemarv42
// Commercial use only with written permission, see LICENSING.md
// RLG OS - Relay Lasting Guard

#include <Arduino.h>
#include <Adafruit_GFX.h>     // tested with 1.11.9
#include <Adafruit_SH110X.h>  // tested with 2.1.10
#include <Wire.h>

#include "config.h"
#include "display.h"
#include "error_codes.h"
#include "input.h"
#include "measure.h"
#include "pins.h"
#include "relay_test.h"
#include "timekeeping.h"

static constexpr int W = 128;
static constexpr int H = 64;

static Adafruit_SH1106G oled(W, H, &Wire, -1);
static int activityDots = 0;

// ================================ Helpers ================================

static void beginScreen(const char* title) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println(title);
  oled.drawFastHLine(0, 10, W, SH110X_WHITE);
}

static void textAt(int x, int y, const char* text) {
  oled.setCursor(x, y);
  oled.print(text);
}

static void drawMenu(const char* const* items, int count, int selected, int y0, int pitch) {
  for (int i = 0; i < count; i++) {
    const bool sel = (i == selected);
    if (sel) oled.fillRect(0, y0 + i * pitch, W, pitch - 2, SH110X_WHITE);
    oled.setTextColor(sel ? SH110X_BLACK : SH110X_WHITE);
    textAt(4, y0 + 3 + i * pitch, items[i]);
  }
  oled.setTextColor(SH110X_WHITE);
}

// ================================ Screens ================================

void DisplayInit() {
  oled.begin(OLED_I2C_ADDRESS, true);
  oled.clearDisplay();
  oled.display();
}

void DrawBootScreen() {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setTextSize(2);
  textAt(28, 2, "RLG");
  oled.drawFastHLine(0, 21, W, SH110X_WHITE);
  oled.setTextSize(1);
  textAt(0, 25, "Relay Lasting Guard");
  textAt(0, 37, "by Marvin Ramm");
  textAt(0, 49, "Version " RLG_VERSION);
  oled.display();
}

void DrawFuseScreen() {
  beginScreen("!! Enable main fuse");
  textAt(0, 15, "Switch on the slave");
  textAt(0, 25, "fuse, then press");
  textAt(0, 35, "the encoder button.");
  oled.display();
}

void DrawModeSelect(int selected) {
  static const char* const modes[] = {"Lifespan", "Contact Res.", "ZC Oscilloscope"};
  beginScreen("-- Select Mode --");
  drawMenu(modes, 3, selected, 14, 16);
  oled.display();
}

void DrawScopeConfirm() {
  beginScreen("ZC Oscilloscope");
  textAt(0, 14, "Insert reference");
  textAt(0, 24, "relay into RELAY1.");
  textAt(0, 38, "Press encoder = start");
  oled.display();
}

void DrawScopeSubMenu(int selected) {
  static const char* const items[] = {"Free-run", "ZC-sync"};
  beginScreen("ZC Oscilloscope");
  drawMenu(items, 2, selected, 14, 18);
  oled.display();
}

void DrawAskClock() {
  beginScreen("Set date & time?");
  textAt(0, 16, "Press encoder = YES");
  textAt(0, 28, "Wait 5s      = NO");
  oled.display();
}

void DrawSetTime(int field, const int v[6]) {
  static const char* const labels[6] = {"H:", "M:", "S:", "D:", "Mo:", "Y:"};
  static const int col[6] = {0, 44, 88, 0, 44, 84};
  static const int row[6] = {14, 14, 14, 30, 30, 30};

  beginScreen("Set Date & Time:");
  for (int i = 0; i < 6; i++) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%s%02d", labels[i], (i == 5) ? v[i] % 100 : v[i]);
    if (i == field) {
      oled.fillRect(col[i] - 1, row[i] - 1, (int)strlen(buf) * 6 + 1, 10, SH110X_WHITE);
      oled.setTextColor(SH110X_BLACK);
    }
    textAt(col[i], row[i], buf);
    oled.setTextColor(SH110X_WHITE);
  }
  textAt(0, 46, "Turn: change value");
  textAt(0, 56, "Press: next field");
  oled.display();
}

void DrawNoClockWarning() {
  beginScreen("!! NO CLOCK SET !!");
  textAt(0, 14, "Timestamps will be");
  textAt(0, 24, "missing in log file.");
  textAt(0, 38, "Press again = start");
  textAt(0, 50, "Wait 4s    = cancel");
  oled.display();
}

void DrawContactMeasurementPause() {
  beginScreen(">> PAUSED <<");
  textAt(0, 14, "Measure contact res:");
  oled.setCursor(0, 26);
  if (ContactResNeeded[0] && ContactResNeeded[1]) {
    oled.print("R1 + R2 needed!");
  } else if (ContactResNeeded[0] || ContactResNeeded[1]) {
    const int i = ContactResNeeded[0] ? 0 : 1;
    oled.printf("%s @ %lu ops", Channels[i].tag, (unsigned long)Channels[i].switches);
  } else {
    oled.print("(check serial)");
  }
  textAt(0, 42, "Press encoder: resume");
  oled.display();
}

void DrawSwitchTimeMeasuring(int idx) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  if (idx >= 0) oled.printf("Switch Time - %s", Channels[idx].tag);
  else          oled.print("Switch Time - View");
  oled.drawFastHLine(0, 10, W, SH110X_WHITE);
  textAt(0, 14, "Measuring...");
  textAt(0, 28, "Reference socket");
  oled.display();
}

void DrawSwitchTimeResult(uint32_t onUs, uint32_t offUs, bool saved) {
  beginScreen("Switch Time");
  textAt(0, 14, saved ? "Measured (saved):" : "Measured (not saved):");
  oled.setCursor(0, 28); oled.printf("ON:  %lu us", (unsigned long)onUs);
  oled.setCursor(0, 38); oled.printf("OFF: %lu us", (unsigned long)offUs);
  oled.display();
}

// Header: activity indicator + clock (or page number if no clock is set)
static void drawStatusHeader(int page) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  if (TestRunning) {
    static const char* const dots[] = {"Active", "Active.", "Active..", "Active..."};
    oled.print(dots[activityDots]);
    activityDots = (activityDots + 1) % 4;
  } else {
    oled.print("Idle");
  }

  char right[16];
  if (ClockIsSet()) snprintf(right, sizeof(right), "%s", ClockNow().substring(11).c_str());
  else              snprintf(right, sizeof(right), "%d/4", page + 1);
  textAt(W - (int)strlen(right) * 6, 0, right);
  oled.drawFastHLine(0, 9, W, SH110X_WHITE);
}

// Two blocks of three lines (one per relay), separated by a line at y = 43.
static void drawChannelBlock(int idx, int y) {
  const TestChannel& ch = Channels[idx];
  oled.setCursor(0, y);
  oled.printf("%s[0x%lX]%s", ch.tag, (unsigned long)ch.id, ch.enabled ? "" : " OFF");
  oled.setCursor(0, y + 10);
  // max. 21 characters: " Cnt:1302480 t:589.5h"
  oled.printf(" Cnt:%lu t:%.1fh", (unsigned long)ch.switches,
              RelayTimerElapsedMs(idx) / 3600000.0);
}

void DrawStatusPage(int page) {
  EncMaxPos = 3;
  drawStatusHeader(page);

  switch (page) {
    case 0:   // cycle counters and elapsed time
      drawChannelBlock(0, 12);
      oled.drawFastHLine(0, 43, W, SH110X_WHITE);
      drawChannelBlock(1, 46);
      break;

    case 1:   // temperatures and switching mode
      oled.setCursor(0, 12); oled.printf("Load1: %.1fC%s", TempC[TEMP_LOAD1], TempC[TEMP_LOAD1] > LOAD_MAX_TEMP_C ? " HOT!" : "");
      oled.setCursor(0, 22); oled.printf("Load2: %.1fC%s", TempC[TEMP_LOAD2], TempC[TEMP_LOAD2] > LOAD_MAX_TEMP_C ? " HOT!" : "");
      oled.setCursor(0, 32); oled.printf("Aux:   %.1fC", TempC[TEMP_AUX]);
      oled.drawFastHLine(0, 43, W, SH110X_WHITE);
      oled.setCursor(0, 46); oled.printf("R1 ZC: %s", Channels[0].zeroCross ? "ON" : "OFF");
      oled.setCursor(0, 56); oled.printf("R2 ZC: %s", Channels[1].zeroCross ? "ON" : "OFF");
      break;

    case 2:   // errors, mains frequency, mode
      oled.setCursor(0, 12); oled.printf("Err: 0x%X", (unsigned)ErrorCode);
      oled.setCursor(0, 22); oled.printf("R1 Fails: %lu/%d", (unsigned long)Channels[0].fails, MAX_RELAY_FAILS);
      oled.setCursor(0, 32); oled.printf("R2 Fails: %lu/%d", (unsigned long)Channels[1].fails, MAX_RELAY_FAILS);
      oled.drawFastHLine(0, 43, W, SH110X_WHITE);
      oled.setCursor(0, 46); oled.printf("AC Freq: %d Hz", (int)MainsFrequency);
      oled.setCursor(0, 56); oled.printf("Mode: %s", OperatingMode == MODE_LIFESPAN ? "Lifespan" : "Contact Res");
      break;

    case 3:   // switch times
      oled.setCursor(0, 12); oled.printf("R1 ON:  %luus", (unsigned long)Channels[0].switchTimeOnUs);
      oled.setCursor(0, 22); oled.printf("R1 OFF: %luus", (unsigned long)Channels[0].switchTimeOffUs);
      oled.setCursor(0, 32); oled.printf("Meas: %s", Channels[0].switchTimeMeasured ? "YES" : "NO");
      oled.drawFastHLine(0, 43, W, SH110X_WHITE);
      oled.setCursor(0, 46); oled.printf("R2 ON:  %luus", (unsigned long)Channels[1].switchTimeOnUs);
      oled.setCursor(0, 56); oled.printf("R2 OFF: %luus", (unsigned long)Channels[1].switchTimeOffUs);
      break;
  }
  oled.display();
}
