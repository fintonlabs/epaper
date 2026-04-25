#pragma once
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "icons.h"

// Display dimensions
#define DISP_W 400
#define DISP_H 300

// Forward declaration - display is defined in main.cpp
extern GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display;

// Track last update time
static unsigned long lastUpdateMillis = 0;
static char lastUpdateType[32] = "none";
static int partialRefreshCount = 0;
#define FULL_REFRESH_INTERVAL 300  // Full refresh every ~5 min at 1/sec to clear ghosting

// Use partial refresh for content updates (no flicker).
// Every FULL_REFRESH_INTERVAL updates, do a full refresh to clear ghosting.
void setDisplayWindow(bool forceFullRefresh = false) {
    if (forceFullRefresh || partialRefreshCount >= FULL_REFRESH_INTERVAL) {
        display.setFullWindow();
        partialRefreshCount = 0;
    } else {
        display.setPartialWindow(0, 0, DISP_W, DISP_H);
        partialRefreshCount++;
    }
}

void setLastUpdate(const char* type) {
    lastUpdateMillis = millis();
    strncpy(lastUpdateType, type, sizeof(lastUpdateType) - 1);
    lastUpdateType[sizeof(lastUpdateType) - 1] = '\0';
}

// Set font by size number (1-4)
const GFXfont* getFontBySize(int size) {
    switch (size) {
        case 1: return &FreeSansBold9pt7b;
        case 2: return &FreeSansBold12pt7b;
        case 3: return &FreeSansBold18pt7b;
        case 4: return &FreeSansBold24pt7b;
        default: return &FreeSansBold12pt7b;
    }
}

// Draw an icon at position with optional scaling
void drawIcon(int16_t x, int16_t y, const uint8_t* icon, int scale = 1) {
    if (!icon) return;
    for (int iy = 0; iy < 32; iy++) {
        for (int ix = 0; ix < 32; ix++) {
            int byteIdx = iy * 4 + (ix / 8);
            int bitIdx = 7 - (ix % 8);
            if (pgm_read_byte(&icon[byteIdx]) & (1 << bitIdx)) {
                if (scale == 1) {
                    display.drawPixel(x + ix, y + iy, GxEPD_BLACK);
                } else {
                    display.fillRect(x + ix * scale, y + iy * scale, scale, scale, GxEPD_BLACK);
                }
            }
        }
    }
}

// ---- TEXT RENDERING ----
void renderText(const char* text, int size, int x, int y, const char* align) {
    const GFXfont* font = getFontBySize(size);
    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

    int16_t cx, cy;

    if (strcmp(align, "center") == 0) {
        cx = (DISP_W - tbw) / 2 - tbx;
        cy = y > 0 ? y : (DISP_H - tbh) / 2 - tby;
    } else if (strcmp(align, "right") == 0) {
        cx = DISP_W - tbw - 10 - tbx;
        cy = y > 0 ? y : (DISP_H - tbh) / 2 - tby;
    } else {
        cx = x > 0 ? x : 10;
        cy = y > 0 ? y : (DISP_H - tbh) / 2 - tby;
    }

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(cx, cy);
        display.print(text);
    } while (display.nextPage());

    setLastUpdate("text");
}

// Word-wrap text drawing helper (doesn't manage pages)
void drawWrappedText(const char* text, int16_t x, int16_t y, int16_t maxWidth, const GFXfont* font) {
    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);

    int16_t curX = x, curY = y;
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    // Get line height
    display.getTextBounds("Ag", 0, 0, &tbx, &tby, &tbw, &tbh);
    int lineHeight = tbh + 4;

    char word[64];
    int wi = 0;
    const char* p = text;

    while (*p) {
        if (*p == ' ' || *p == '\n' || *(p + 1) == '\0') {
            if (*(p + 1) == '\0' && *p != ' ' && *p != '\n') {
                word[wi++] = *p;
            }
            word[wi] = '\0';
            if (wi > 0) {
                display.getTextBounds(word, 0, 0, &tbx, &tby, &tbw, &tbh);
                if (curX + tbw > x + maxWidth && curX > x) {
                    curX = x;
                    curY += lineHeight;
                }
                display.setCursor(curX, curY);
                display.print(word);
                display.print(" ");
                curX += tbw + 6;
            }
            if (*p == '\n') {
                curX = x;
                curY += lineHeight;
            }
            wi = 0;
        } else {
            if (wi < 62) word[wi++] = *p;
        }
        p++;
    }
}

// ---- NOTIFICATION RENDERING ----
void renderNotification(const char* title, const char* body, const char* iconName, const char* style) {
    const uint8_t* iconData = getIcon(iconName);

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (strcmp(style, "fullscreen") == 0) {
            // Full screen notification
            if (iconData) {
                drawIcon((DISP_W - 64) / 2, 30, iconData, 2);
            }
            display.setFont(&FreeSansBold18pt7b);
            display.setTextColor(GxEPD_BLACK);
            int16_t tbx, tby; uint16_t tbw, tbh;
            display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
            display.setCursor((DISP_W - tbw) / 2 - tbx, iconData ? 140 : 100);
            display.print(title);

            drawWrappedText(body, 20, iconData ? 190 : 160, DISP_W - 40, &FreeSansBold9pt7b);

        } else if (strcmp(style, "banner") == 0) {
            // Top banner
            display.fillRect(0, 0, DISP_W, 60, GxEPD_BLACK);
            display.setFont(&FreeSansBold12pt7b);
            display.setTextColor(GxEPD_WHITE);
            int16_t iconOffset = 10;
            if (iconData) {
                // Draw icon inverted in banner
                for (int iy = 0; iy < 32; iy++) {
                    for (int ix = 0; ix < 32; ix++) {
                        int byteIdx = iy * 4 + (ix / 8);
                        int bitIdx = 7 - (ix % 8);
                        if (pgm_read_byte(&iconData[byteIdx]) & (1 << bitIdx)) {
                            display.drawPixel(12 + ix, 14 + iy, GxEPD_WHITE);
                        }
                    }
                }
                iconOffset = 52;
            }
            display.setCursor(iconOffset, 40);
            display.print(title);

            display.setTextColor(GxEPD_BLACK);
            drawWrappedText(body, 12, 90, DISP_W - 24, &FreeSansBold12pt7b);

        } else {
            // Card style (centered box)
            int cardW = 360, cardH = 200;
            int cardX = (DISP_W - cardW) / 2;
            int cardY = (DISP_H - cardH) / 2;
            display.drawRect(cardX, cardY, cardW, cardH, GxEPD_BLACK);
            display.drawRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2, GxEPD_BLACK);

            // Title bar
            display.fillRect(cardX, cardY, cardW, 45, GxEPD_BLACK);
            display.setFont(&FreeSansBold12pt7b);
            display.setTextColor(GxEPD_WHITE);
            int16_t iconOffset = cardX + 10;
            if (iconData) {
                for (int iy = 0; iy < 32; iy++) {
                    for (int ix = 0; ix < 32; ix++) {
                        int byteIdx = iy * 4 + (ix / 8);
                        int bitIdx = 7 - (ix % 8);
                        if (pgm_read_byte(&iconData[byteIdx]) & (1 << bitIdx)) {
                            display.drawPixel(cardX + 8 + ix, cardY + 7 + iy, GxEPD_WHITE);
                        }
                    }
                }
                iconOffset = cardX + 48;
            }
            display.setCursor(iconOffset, cardY + 32);
            display.print(title);

            display.setTextColor(GxEPD_BLACK);
            drawWrappedText(body, cardX + 12, cardY + 70, cardW - 24, &FreeSansBold9pt7b);
        }
    } while (display.nextPage());

    setLastUpdate("notification");
}

// ---- DASHBOARD RENDERING ----
void renderDashboard(JsonArray widgets) {
    int count = widgets.size();
    if (count == 0) return;
    if (count > 6) count = 6;

    // Calculate grid layout
    int cols, rows;
    if (count <= 2) { cols = 2; rows = 1; }
    else if (count <= 4) { cols = 2; rows = 2; }
    else { cols = 3; rows = 2; }

    int cellW = DISP_W / cols;
    int cellH = DISP_H / rows;

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        for (int i = 0; i < count; i++) {
            JsonObject widget = widgets[i];
            int col = i % cols;
            int row = i / cols;
            int cx = col * cellW;
            int cy = row * cellH;

            // Draw cell border
            display.drawRect(cx, cy, cellW, cellH, GxEPD_BLACK);

            const char* type = widget["type"] | "metric";
            const char* label = widget["label"] | "";
            const char* value = widget["value"] | "";
            const char* content = widget["content"] | "";
            const char* iconName = widget["icon"] | "";

            if (strcmp(type, "metric") == 0) {
                // Icon
                const uint8_t* iconData = getIcon(iconName);
                int textStartY = cy + 10;
                if (iconData) {
                    drawIcon(cx + (cellW - 32) / 2, cy + 8, iconData);
                    textStartY = cy + 48;
                }

                // Value (large)
                display.setFont(&FreeSansBold18pt7b);
                display.setTextColor(GxEPD_BLACK);
                int16_t tbx, tby; uint16_t tbw, tbh;
                display.getTextBounds(value, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor(cx + (cellW - tbw) / 2 - tbx, textStartY + tbh + 4);
                display.print(value);

                // Label (small)
                display.setFont(&FreeSansBold9pt7b);
                display.getTextBounds(label, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor(cx + (cellW - tbw) / 2 - tbx, cy + cellH - 10);
                display.print(label);

            } else if (strcmp(type, "text") == 0) {
                drawWrappedText(content, cx + 8, cy + 24, cellW - 16, &FreeSansBold9pt7b);
            }
        }
    } while (display.nextPage());

    setLastUpdate("dashboard");
}

// ---- EMOJI RENDERING ----
void renderEmoji(const char* emojiName, const char* sizeMode, const char* caption) {
    const uint8_t* iconData = getIcon(emojiName);
    if (!iconData) {
        // Fallback: show name as text
        renderText(emojiName, 3, 0, 0, "center");
        return;
    }

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (strcmp(sizeMode, "fullscreen") == 0) {
            // Draw icon scaled to fill most of screen
            int scale = 7; // 32*7 = 224 pixels
            int ox = (DISP_W - 32 * scale) / 2;
            int oy = caption[0] ? 10 : (DISP_H - 32 * scale) / 2;
            drawIcon(ox, oy, iconData, scale);

            if (caption[0]) {
                display.setFont(&FreeSansBold18pt7b);
                display.setTextColor(GxEPD_BLACK);
                int16_t tbx, tby; uint16_t tbw, tbh;
                display.getTextBounds(caption, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor((DISP_W - tbw) / 2 - tbx, DISP_H - 15);
                display.print(caption);
            }
        } else {
            // Normal size (centered 64x64)
            int scale = 2;
            int ox = (DISP_W - 32 * scale) / 2;
            int oy = caption[0] ? (DISP_H - 64 - 40) / 2 : (DISP_H - 64) / 2;
            drawIcon(ox, oy, iconData, scale);

            if (caption[0]) {
                display.setFont(&FreeSansBold12pt7b);
                display.setTextColor(GxEPD_BLACK);
                int16_t tbx, tby; uint16_t tbw, tbh;
                display.getTextBounds(caption, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor((DISP_W - tbw) / 2 - tbx, oy + 64 + 30 + tbh);
                display.print(caption);
            }
        }
    } while (display.nextPage());

    setLastUpdate("emoji");
}

// ---- WEATHER RENDERING ----
void renderWeather(const char* temp, const char* condition, const char* humidity, const char* location) {
    // Map condition to icon
    const char* iconName = "sun";
    if (strstr(condition, "cloud") || strstr(condition, "overcast")) iconName = "cloud";
    else if (strstr(condition, "rain") || strstr(condition, "drizzle")) iconName = "rain";
    else if (strstr(condition, "snow") || strstr(condition, "sleet")) iconName = "snow";
    else if (strstr(condition, "storm") || strstr(condition, "thunder")) iconName = "storm";
    else if (strstr(condition, "sun") || strstr(condition, "clear")) iconName = "sun";

    const uint8_t* iconData = getIcon(iconName);

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Location header
        display.fillRect(0, 0, DISP_W, 40, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_WHITE);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(location, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 30);
        display.print(location);

        // Weather icon (large)
        if (iconData) {
            drawIcon(30, 60, iconData, 4); // 128x128
        }

        // Temperature (huge)
        display.setTextColor(GxEPD_BLACK);
        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%s°", temp);
        display.setFont(&FreeSansBold24pt7b);
        display.getTextBounds(tempStr, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor(200, 130);
        display.print(tempStr);

        // Condition
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(200, 170);
        display.print(condition);

        // Humidity
        if (humidity[0]) {
            char humStr[32];
            snprintf(humStr, sizeof(humStr), "Humidity: %s%%", humidity);
            display.setFont(&FreeSansBold9pt7b);
            display.setCursor(200, 200);
            display.print(humStr);
        }

        // Bottom divider
        display.drawFastHLine(10, DISP_H - 50, DISP_W - 20, GxEPD_BLACK);

        // Thermometer icon + temp at bottom
        const uint8_t* thermIcon = getIcon("thermometer");
        if (thermIcon) {
            drawIcon(10, DISP_H - 42, thermIcon);
        }
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(50, DISP_H - 18);
        char bottomText[64];
        snprintf(bottomText, sizeof(bottomText), "%s°  |  %s  |  %s%%", temp, condition, humidity);
        display.print(bottomText);

    } while (display.nextPage());

    setLastUpdate("weather");
}

// ---- COUNTDOWN RENDERING ----
void renderCountdown(const char* title, long secondsRemaining) {
    int days = secondsRemaining / 86400;
    int hours = (secondsRemaining % 86400) / 3600;
    int mins = (secondsRemaining % 3600) / 60;
    int secs = secondsRemaining % 60;

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Title
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(title, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 50);
        display.print(title);

        // Clock icon
        const uint8_t* clockIcon = getIcon("clock");
        if (clockIcon) {
            drawIcon((DISP_W - 64) / 2, 70, clockIcon, 2);
        }

        // Countdown value
        char countStr[32];
        if (days > 0) {
            snprintf(countStr, sizeof(countStr), "%dd %02d:%02d:%02d", days, hours, mins, secs);
        } else {
            snprintf(countStr, sizeof(countStr), "%02d:%02d:%02d", hours, mins, secs);
        }
        display.setFont(&FreeSansBold24pt7b);
        display.getTextBounds(countStr, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 220);
        display.print(countStr);

        if (secondsRemaining <= 0) {
            display.setFont(&FreeSansBold18pt7b);
            const char* done = "COMPLETE";
            display.getTextBounds(done, 0, 0, &tbx, &tby, &tbw, &tbh);
            display.setCursor((DISP_W - tbw) / 2 - tbx, 270);
            display.print(done);
        }

    } while (display.nextPage());

    setLastUpdate("countdown");
}

// ---- CLOCK RENDERING ----
struct ClockZone {
    char label[32];
    int offsetMinutes;  // UTC offset in minutes
};

void renderClock(ClockZone* zones, int zoneCount, bool hour24) {
    time_t now = time(nullptr);

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        if (zoneCount <= 0 || now < 100000) {
            // No NTP time yet
            display.setFont(&FreeSansBold18pt7b);
            display.setTextColor(GxEPD_BLACK);
            display.setCursor(60, 160);
            display.print("Waiting for NTP...");
            display.nextPage();
            return;
        }

        // Primary timezone - large display
        time_t localTime = now + (long)zones[0].offsetMinutes * 60L;
        struct tm* t = gmtime(&localTime);

        // Header bar with label
        display.fillRect(0, 0, DISP_W, 42, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(12, 30);
        display.print(zones[0].label);

        // Clock icon in header
        const uint8_t* clockIcon = getIcon("clock");
        if (clockIcon) {
            for (int iy = 0; iy < 32; iy++) {
                for (int ix = 0; ix < 32; ix++) {
                    int byteIdx = iy * 4 + (ix / 8);
                    int bitIdx = 7 - (ix % 8);
                    if (pgm_read_byte(&clockIcon[byteIdx]) & (1 << bitIdx)) {
                        display.drawPixel(360 + ix, 5 + iy, GxEPD_WHITE);
                    }
                }
            }
        }

        display.setTextColor(GxEPD_BLACK);

        // Big time
        char timeBuf[16];
        if (hour24) {
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        } else {
            int h = t->tm_hour % 12;
            if (h == 0) h = 12;
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d:%02d", h, t->tm_min, t->tm_sec);
        }

        display.setFont(&FreeSansBold24pt7b);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(timeBuf, 0, 0, &tbx, &tby, &tbw, &tbh);

        int primaryY = (zoneCount == 1) ? 160 : 110;
        display.setCursor((DISP_W - tbw) / 2 - tbx, primaryY);
        display.print(timeBuf);

        // AM/PM for 12-hour mode
        if (!hour24) {
            display.setFont(&FreeSansBold12pt7b);
            display.setCursor((DISP_W + tbw) / 2 + 8, primaryY);
            display.print(t->tm_hour >= 12 ? "PM" : "AM");
        }

        // Date under primary time
        char dateBuf[32];
        const char* dayNames[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        const char* monNames[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        snprintf(dateBuf, sizeof(dateBuf), "%s %d %s %d",
            dayNames[t->tm_wday], t->tm_mday, monNames[t->tm_mon], t->tm_year + 1900);
        display.setFont(&FreeSansBold9pt7b);
        display.getTextBounds(dateBuf, 0, 0, &tbx, &tby, &tbw, &tbh);
        int dateY = (zoneCount == 1) ? 190 : 140;
        display.setCursor((DISP_W - tbw) / 2 - tbx, dateY);
        display.print(dateBuf);

        // Additional timezones
        if (zoneCount > 1) {
            display.drawFastHLine(20, 160, 360, GxEPD_BLACK);

            int slotCount = zoneCount - 1;
            if (slotCount > 3) slotCount = 3;
            int slotW = DISP_W / slotCount;

            for (int i = 0; i < slotCount; i++) {
                int zi = i + 1;
                time_t zt = now + (long)zones[zi].offsetMinutes * 60L;
                struct tm* tz = gmtime(&zt);

                int cx = i * slotW + slotW / 2;

                // Zone label
                display.setFont(&FreeSansBold9pt7b);
                display.getTextBounds(zones[zi].label, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor(cx - tbw / 2 - tbx, 185);
                display.print(zones[zi].label);

                // Zone time
                char ztBuf[16];
                if (hour24) {
                    snprintf(ztBuf, sizeof(ztBuf), "%02d:%02d:%02d", tz->tm_hour, tz->tm_min, tz->tm_sec);
                } else {
                    int zh = tz->tm_hour % 12;
                    if (zh == 0) zh = 12;
                    snprintf(ztBuf, sizeof(ztBuf), "%d:%02d %s", zh, tz->tm_min, tz->tm_hour >= 12 ? "PM" : "AM");
                }
                display.setFont(&FreeSansBold18pt7b);
                display.getTextBounds(ztBuf, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor(cx - tbw / 2 - tbx, 230);
                display.print(ztBuf);

                // Zone date (short)
                char zdBuf[16];
                snprintf(zdBuf, sizeof(zdBuf), "%s %d", dayNames[tz->tm_wday], tz->tm_mday);
                display.setFont(&FreeMono9pt7b);
                display.getTextBounds(zdBuf, 0, 0, &tbx, &tby, &tbw, &tbh);
                display.setCursor(cx - tbw / 2 - tbx, 260);
                display.print(zdBuf);

                // Vertical dividers between zones
                if (i < slotCount - 1) {
                    display.drawFastVLine((i + 1) * slotW, 165, 110, GxEPD_BLACK);
                }
            }
        }

        // Footer line
        int footerY = (zoneCount == 1) ? 260 : 275;
        display.drawFastHLine(20, footerY, 360, GxEPD_BLACK);
        display.setFont(&FreeMono9pt7b);
        char utcBuf[24];
        struct tm* utc = gmtime(&now);
        snprintf(utcBuf, sizeof(utcBuf), "UTC %02d:%02d:%02d", utc->tm_hour, utc->tm_min, utc->tm_sec);
        display.setCursor(20, footerY + 20);
        display.print(utcBuf);

    } while (display.nextPage());

    setLastUpdate("clock");
}

// ---- CLEAR DISPLAY ----
void renderClear() {
    setDisplayWindow(true);  // Full refresh to clear ghosting
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());

    setLastUpdate("clear");
}

// ---- QR CODE RENDERING (using inline generation) ----
void renderQRCode(const char* data, const char* caption) {
    // QR code generation is done in main.cpp using the qrcode library
    // This is a placeholder - actual rendering is in the API handler
    setLastUpdate("qrcode");
}

// ---- AP SETUP SCREEN ----
void renderAPSetupScreen(const char* apName, const char* ipAddr) {
    setDisplayWindow(true);  // Full refresh on boot
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Header bar
        display.fillRect(0, 0, DISP_W, 45, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(12, 32);
        display.print("WiFi Setup Required");

        // WiFi icon in header
        const uint8_t* wifiIcon = getIcon("wifi");
        if (wifiIcon) {
            for (int iy = 0; iy < 32; iy++) {
                for (int ix = 0; ix < 32; ix++) {
                    int byteIdx = iy * 4 + (ix / 8);
                    int bitIdx = 7 - (ix % 8);
                    if (pgm_read_byte(&wifiIcon[byteIdx]) & (1 << bitIdx)) {
                        display.drawPixel(360 + ix, 7 + iy, GxEPD_WHITE);
                    }
                }
            }
        }

        display.setTextColor(GxEPD_BLACK);

        // AP name
        display.setFont(&FreeSansBold18pt7b);
        display.setCursor(30, 95);
        display.print("Connect to:");

        display.setFont(&FreeSansBold24pt7b);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(apName, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 140);
        display.print(apName);

        // Divider
        display.drawFastHLine(20, 160, 360, GxEPD_BLACK);

        // Instructions
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(20, 190);
        display.print("Then open browser to:");

        display.setFont(&FreeSansBold18pt7b);
        char url[32];
        snprintf(url, sizeof(url), "http://%s", ipAddr);
        display.getTextBounds(url, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 230);
        display.print(url);

        // Footer
        display.drawFastHLine(20, 260, 360, GxEPD_BLACK);
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(20, 285);
        display.print("No password required | Portal opens automatically");

    } while (display.nextPage());
    setLastUpdate("ap_setup");
}

// ---- WIFI CONNECTING SCREEN ----
void renderWiFiConnecting(const char* ssid) {
    setDisplayWindow(true);  // Full refresh on boot
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // WiFi icon centered
        const uint8_t* wifiIcon = getIcon("wifi");
        if (wifiIcon) {
            drawIcon((DISP_W - 64) / 2, 60, wifiIcon, 2);
        }

        // "Connecting to..." text
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        int16_t tbx, tby; uint16_t tbw, tbh;
        const char* label = "Connecting to";
        display.getTextBounds(label, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 165);
        display.print(label);

        // SSID name
        display.setFont(&FreeSansBold18pt7b);
        display.getTextBounds(ssid, 0, 0, &tbx, &tby, &tbw, &tbh);
        display.setCursor((DISP_W - tbw) / 2 - tbx, 210);
        display.print(ssid);

        // Footer
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(100, 270);
        display.print("Please wait...");

    } while (display.nextPage());
    setLastUpdate("wifi_connecting");
}

// ---- READY SCREEN ----
void renderReadyScreen() {
    setDisplayWindow(true);  // Full refresh on boot
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        // Header bar
        display.fillRect(0, 0, DISP_W, 45, GxEPD_BLACK);
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(12, 32);
        display.print("E-Paper Display Ready");

        // WiFi icon
        const uint8_t* wifiIcon = getIcon("wifi");
        if (wifiIcon) {
            for (int iy = 0; iy < 32; iy++) {
                for (int ix = 0; ix < 32; ix++) {
                    int byteIdx = iy * 4 + (ix / 8);
                    int bitIdx = 7 - (ix % 8);
                    if (pgm_read_byte(&wifiIcon[byteIdx]) & (1 << bitIdx)) {
                        display.drawPixel(360 + ix, 7 + iy, GxEPD_WHITE);
                    }
                }
            }
        }

        display.setTextColor(GxEPD_BLACK);

        // IP address
        display.setFont(&FreeSansBold18pt7b);
        String ip = WiFi.localIP().toString();
        display.setCursor(30, 100);
        display.print(ip);

        // mDNS
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(30, 135);
        display.print("epaper.local");

        // Divider
        display.drawFastHLine(20, 155, 360, GxEPD_BLACK);

        // Instructions
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(20, 180);
        display.print("Open in browser to control display");
        display.setCursor(20, 205);
        display.print("REST API available at /api/*");

        // RSSI and port
        display.setFont(&FreeMono9pt7b);
        char info[64];
        snprintf(info, sizeof(info), "RSSI: %d dBm  |  Port: 80", WiFi.RSSI());
        display.setCursor(20, 240);
        display.print(info);

        // Footer
        display.drawFastHLine(20, 260, 360, GxEPD_BLACK);
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(20, 285);
        display.print("WeAct 4.2\" SSD1683 | ESP32-S3");

    } while (display.nextPage());
    setLastUpdate("boot");
}
