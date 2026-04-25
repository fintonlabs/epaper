#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <qrcode.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// ---- Pin mapping ----
#define EPD_MOSI  11
#define EPD_SCK   12
#define EPD_CS    10
#define EPD_DC     9
#define EPD_RST    8
#define EPD_BUSY   7

// ---- Display ----
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>
    display(GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// ---- Includes that depend on display type ----
#include "icons.h"
#include "display_renderer.h"
#include "wifi_manager.h"
#include "web_ui.h"

// ---- Server ----
AsyncWebServer server(80);

// ---- Countdown state ----
static bool countdownActive = false;
static unsigned long countdownTargetMillis = 0;
static char countdownTitle[64] = "";
static unsigned long lastCountdownUpdate = 0;

// ---- Clock state ----
static bool clockActive = false;
static ClockZone clockZones[4];
static int clockZoneCount = 0;
static bool clockHour24 = true;
static unsigned long lastClockUpdate = 0;

// ---- Config ----
static int displayRotation = 0;
static unsigned long sleepAfterSec = 0;
static bool deepSleepEnabled = false;

// ---- Uptime ----
static unsigned long bootTime = 0;

// ---- Helper: parse JSON body ----
static char jsonBuf[4096];
static JsonDocument jsonDoc;

JsonDocument* parseBody(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
    if (len >= sizeof(jsonBuf)) len = sizeof(jsonBuf) - 1;
    memcpy(jsonBuf, data, len);
    jsonBuf[len] = '\0';
    Serial.printf("JSON: %s\n", jsonBuf);

    jsonDoc.clear();
    DeserializationError err = deserializeJson(jsonDoc, jsonBuf);
    if (err) {
        Serial.printf("JSON parse error: %s\n", err.c_str());
        return nullptr;
    }
    return &jsonDoc;
}

// ---- QR code rendering ----
void renderQROnDisplay(const char* qrData, const char* caption) {
    int version = 3;
    int dataLen = strlen(qrData);
    if (dataLen > 77) version = 5;
    if (dataLen > 134) version = 8;

    QRCode qrcode;
    int bufSize = qrcode_getBufferSize(version);
    uint8_t* qrcodeData = (uint8_t*)malloc(bufSize);
    if (!qrcodeData) return;

    int err = qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, qrData);
    if (err != 0) {
        free(qrcodeData);
        return;
    }

    int maxDim = caption[0] ? 240 : 260;
    int pixelSize = maxDim / qrcode.size;
    if (pixelSize < 2) pixelSize = 2;

    int totalSize = qrcode.size * pixelSize;
    int startX = (400 - totalSize) / 2;
    int startY = caption[0] ? (260 - totalSize) / 2 : (300 - totalSize) / 2;

    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        display.fillRect(startX - 4, startY - 4, totalSize + 8, totalSize + 8, GxEPD_WHITE);

        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    display.fillRect(startX + x * pixelSize, startY + y * pixelSize,
                                     pixelSize, pixelSize, GxEPD_BLACK);
                }
            }
        }

        if (caption[0]) {
            display.setFont(&FreeSansBold12pt7b);
            display.setTextColor(GxEPD_BLACK);
            int16_t tbx, tby; uint16_t tbw, tbh;
            display.getTextBounds(caption, 0, 0, &tbx, &tby, &tbw, &tbh);
            display.setCursor((400 - tbw) / 2 - tbx, 290);
            display.print(caption);
        }
    } while (display.nextPage());

    free(qrcodeData);
    setLastUpdate("qrcode");
}

// ---- Image rendering ----
void renderImageFromBuffer(uint8_t* pixels, int imgW, int imgH) {
    setDisplayWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        float scaleX = (float)400 / imgW;
        float scaleY = (float)300 / imgH;
        float scale = min(scaleX, scaleY);
        int dstW = imgW * scale;
        int dstH = imgH * scale;
        int offX = (400 - dstW) / 2;
        int offY = (300 - dstH) / 2;

        for (int dy = 0; dy < dstH; dy++) {
            for (int dx = 0; dx < dstW; dx++) {
                int sx = dx / scale;
                int sy = dy / scale;
                if (sx >= imgW || sy >= imgH) continue;
                uint8_t gray = pixels[sy * imgW + sx];
                if (gray < 128) {
                    display.drawPixel(offX + dx, offY + dy, GxEPD_BLACK);
                }
            }
        }
    } while (display.nextPage());

    setLastUpdate("image");
}

// ---- Setup routes (station mode) ----
void setupRoutes() {
    // Serve web UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", WEB_UI_HTML);
    });

    // Status - includes SSID for System tab
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = (millis() - bootTime) / 1000;
        doc["heap"] = ESP.getFreeHeap();
        doc["last_update"] = lastUpdateType;
        doc["ssid"] = wifiGetSSID();
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Text
    server.on("/api/text", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char txtBuf[512];
            static char alignBuf[16];
            strncpy(txtBuf, (*doc)["text"] | "Hello", sizeof(txtBuf) - 1); txtBuf[sizeof(txtBuf)-1] = '\0';
            int size = (*doc)["size"] | 2;
            int x = (*doc)["x"] | 0;
            int y = (*doc)["y"] | 0;
            strncpy(alignBuf, (*doc)["align"] | "center", sizeof(alignBuf) - 1); alignBuf[sizeof(alignBuf)-1] = '\0';

            Serial.printf("Text: '%s' size=%d align=%s\n", txtBuf, size, alignBuf);
            countdownActive = false;
            clockActive = false;
            renderText(txtBuf, size, x, y, alignBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Notification
    server.on("/api/notification", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char nTitle[128], nBody[512], nIcon[32], nStyle[16];
            strncpy(nTitle, (*doc)["title"] | "Notification", sizeof(nTitle)-1); nTitle[sizeof(nTitle)-1]='\0';
            strncpy(nBody, (*doc)["body"] | "", sizeof(nBody)-1); nBody[sizeof(nBody)-1]='\0';
            strncpy(nIcon, (*doc)["icon"] | "bell", sizeof(nIcon)-1); nIcon[sizeof(nIcon)-1]='\0';
            strncpy(nStyle, (*doc)["style"] | "card", sizeof(nStyle)-1); nStyle[sizeof(nStyle)-1]='\0';

            Serial.printf("Notification: title='%s' icon=%s style=%s\n", nTitle, nIcon, nStyle);
            countdownActive = false;
            clockActive = false;
            renderNotification(nTitle, nBody, nIcon, nStyle);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Dashboard
    server.on("/api/dashboard", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            JsonArray widgets = (*doc)["widgets"].as<JsonArray>();
            if (widgets.isNull() || widgets.size() == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"No widgets\"}");
                return;
            }

            countdownActive = false;
            clockActive = false;
            renderDashboard(widgets);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Emoji
    server.on("/api/emoji", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char eName[32], eSize[16], eCap[128];
            strncpy(eName, (*doc)["emoji"] | "cat", sizeof(eName)-1); eName[sizeof(eName)-1]='\0';
            strncpy(eSize, (*doc)["size"] | "fullscreen", sizeof(eSize)-1); eSize[sizeof(eSize)-1]='\0';
            strncpy(eCap, (*doc)["caption"] | "", sizeof(eCap)-1); eCap[sizeof(eCap)-1]='\0';

            Serial.printf("Emoji: %s size=%s caption='%s'\n", eName, eSize, eCap);
            countdownActive = false;
            clockActive = false;
            renderEmoji(eName, eSize, eCap);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // QR Code
    server.on("/api/qrcode", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char qrBuf[256], qrCap[128];
            strncpy(qrBuf, (*doc)["data"] | "https://example.com", sizeof(qrBuf)-1); qrBuf[sizeof(qrBuf)-1]='\0';
            strncpy(qrCap, (*doc)["caption"] | "", sizeof(qrCap)-1); qrCap[sizeof(qrCap)-1]='\0';

            Serial.printf("QR: data='%s' caption='%s'\n", qrBuf, qrCap);
            countdownActive = false;
            renderQROnDisplay(qrBuf, qrCap);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Weather
    server.on("/api/weather", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char wTemp[16], wCond[32], wHum[16], wLoc[64];
            strncpy(wTemp, (*doc)["temp"] | "20", sizeof(wTemp)-1); wTemp[sizeof(wTemp)-1]='\0';
            strncpy(wCond, (*doc)["condition"] | "sunny", sizeof(wCond)-1); wCond[sizeof(wCond)-1]='\0';
            strncpy(wHum, (*doc)["humidity"] | "", sizeof(wHum)-1); wHum[sizeof(wHum)-1]='\0';
            strncpy(wLoc, (*doc)["location"] | "", sizeof(wLoc)-1); wLoc[sizeof(wLoc)-1]='\0';

            Serial.printf("Weather: %s %s %s%% @ %s\n", wTemp, wCond, wHum, wLoc);
            countdownActive = false;
            clockActive = false;
            renderWeather(wTemp, wCond, wHum, wLoc);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Countdown
    server.on("/api/countdown", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            const char* title = (*doc)["title"] | "Countdown";
            const char* target = (*doc)["target"] | "";

            if (strlen(target) == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Missing target\"}");
                return;
            }

            struct tm tmTarget = {};
            int yr, mo, dy, hr, mn, sc;
            if (sscanf(target, "%d-%d-%dT%d:%d:%d", &yr, &mo, &dy, &hr, &mn, &sc) >= 5) {
                tmTarget.tm_year = yr - 1900;
                tmTarget.tm_mon = mo - 1;
                tmTarget.tm_mday = dy;
                tmTarget.tm_hour = hr;
                tmTarget.tm_min = mn;
                tmTarget.tm_sec = sc;
                time_t targetEpoch = mktime(&tmTarget);
                time_t nowEpoch = time(nullptr);

                long secondsFromNow;
                if (nowEpoch < 100000) {
                    secondsFromNow = 3600;
                    Serial.println("No NTP time available, defaulting to 1h countdown");
                } else {
                    secondsFromNow = (long)(targetEpoch - nowEpoch);
                }
                if (secondsFromNow < 0) secondsFromNow = 0;
                countdownTargetMillis = millis() + (unsigned long)secondsFromNow * 1000UL;
                Serial.printf("Countdown: %ld seconds from now\n", secondsFromNow);
            } else {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid date format\"}");
                return;
            }

            strncpy(countdownTitle, title, sizeof(countdownTitle) - 1);
            countdownTitle[sizeof(countdownTitle) - 1] = '\0';
            countdownActive = true;
            lastCountdownUpdate = 0;

            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Clear
    server.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            countdownActive = false;
            clockActive = false;
            renderClear();
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    server.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        countdownActive = false;
        clockActive = false;
        renderClear();
        request->send(200, "application/json", "{\"ok\":true}");
    });

    // Clock / World Clock
    server.on("/api/clock", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {

            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            clockHour24 = (*doc)["hour24"] | true;
            JsonArray zones = (*doc)["timezones"].as<JsonArray>();
            clockZoneCount = 0;

            if (zones.isNull() || zones.size() == 0) {
                // Default: single UTC clock
                strncpy(clockZones[0].label, "UTC", sizeof(clockZones[0].label));
                clockZones[0].offsetMinutes = 0;
                clockZoneCount = 1;
            } else {
                for (JsonObject z : zones) {
                    if (clockZoneCount >= 4) break;
                    strncpy(clockZones[clockZoneCount].label,
                            z["label"] | "UTC",
                            sizeof(clockZones[clockZoneCount].label) - 1);
                    clockZones[clockZoneCount].label[sizeof(clockZones[0].label) - 1] = '\0';
                    clockZones[clockZoneCount].offsetMinutes = z["offset"] | 0;
                    clockZoneCount++;
                }
            }

            countdownActive = false;
            clockActive = true;
            lastClockUpdate = 0;  // Force immediate render

            Serial.printf("Clock: %d zones, 24h=%d\n", clockZoneCount, clockHour24);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Image upload
    static uint8_t* imgBuf = nullptr;
    static size_t imgSize = 0;
    static size_t imgCapacity = 0;

    server.on("/api/image", HTTP_POST,
        [](AsyncWebServerRequest* request) {
            Serial.printf("Image upload complete: %u bytes received\n", imgSize);
            if (imgBuf && imgSize > 54 && imgBuf[0] == 'B' && imgBuf[1] == 'M') {
                int bmpW = *(int32_t*)(imgBuf + 18);
                int bmpH = abs(*(int32_t*)(imgBuf + 22));
                int bpp = *(int16_t*)(imgBuf + 28);
                uint32_t dataOffset = *(uint32_t*)(imgBuf + 10);
                Serial.printf("BMP: %dx%d, %d bpp, offset=%u\n", bmpW, bmpH, bpp, dataOffset);

                uint8_t* gray = (uint8_t*)malloc(bmpW * bmpH);
                if (gray) {
                    int rowSize = ((bmpW * bpp / 8 + 3) / 4) * 4;
                    bool bottomUp = (*(int32_t*)(imgBuf + 22)) > 0;

                    for (int y = 0; y < bmpH; y++) {
                        int srcY = bottomUp ? (bmpH - 1 - y) : y;
                        uint8_t* row = imgBuf + dataOffset + srcY * rowSize;
                        for (int x = 0; x < bmpW; x++) {
                            if (bpp == 24) {
                                uint8_t b = row[x * 3];
                                uint8_t g = row[x * 3 + 1];
                                uint8_t r = row[x * 3 + 2];
                                gray[y * bmpW + x] = (r * 77 + g * 150 + b * 29) >> 8;
                            } else if (bpp == 1) {
                                int byteIdx = x / 8;
                                int bitIdx = 7 - (x % 8);
                                gray[y * bmpW + x] = (row[byteIdx] & (1 << bitIdx)) ? 255 : 0;
                            } else {
                                gray[y * bmpW + x] = 128;
                            }
                        }
                    }

                    countdownActive = false;
                    clockActive = false;
                    renderImageFromBuffer(gray, bmpW, bmpH);
                    free(gray);
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    Serial.println("Failed to allocate grayscale buffer");
                    request->send(500, "application/json", "{\"ok\":false,\"error\":\"Out of memory\"}");
                }
            } else {
                Serial.printf("Not a valid BMP (size=%u, header=%c%c)\n",
                    imgSize, imgBuf ? (char)imgBuf[0] : '?', imgBuf ? (char)imgBuf[1] : '?');
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid BMP\"}");
            }
            if (imgBuf) { free(imgBuf); imgBuf = nullptr; }
            imgSize = 0;
        },
        [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                Serial.printf("Image upload start: %s, heap=%u\n", filename.c_str(), ESP.getFreeHeap());
                if (imgBuf) { free(imgBuf); imgBuf = nullptr; }
                size_t freeHeap = ESP.getFreeHeap();
                imgCapacity = (freeHeap * 80) / 100;
                if (imgCapacity > 120000) imgCapacity = 120000;
                if (imgCapacity < 1024) imgCapacity = 0;
                imgBuf = imgCapacity ? (uint8_t*)malloc(imgCapacity) : nullptr;
                imgSize = 0;
                Serial.printf("Image buffer: %u bytes allocated\n", imgBuf ? imgCapacity : 0);
                if (!imgBuf) Serial.println("Failed to allocate image buffer!");
            }

            if (imgBuf && imgSize + len <= imgCapacity) {
                memcpy(imgBuf + imgSize, data, len);
                imgSize += len;
            }

            if (final) {
                Serial.printf("Image upload final chunk, total: %u bytes\n", imgSize);
            }
        }
    );

    // Config
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            if (doc->containsKey("rotation")) {
                displayRotation = (*doc)["rotation"] | 0;
                display.setRotation(displayRotation);
            }
            if (doc->containsKey("sleep_after")) {
                sleepAfterSec = (*doc)["sleep_after"] | 0;
            }
            if (doc->containsKey("deep_sleep")) {
                deepSleepEnabled = (*doc)["deep_sleep"] | false;
            }

            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // WiFi reset endpoint (station mode)
    server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            request->send(200, "application/json", "{\"ok\":true}");
            delay(500);
            wifiClearCredentials(); // clears NVS and reboots
        }
    );

    // Also handle wifi reset with no body
    server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"ok\":true}");
        delay(500);
        wifiClearCredentials();
    });

    // CORS preflight
    server.on("/api/*", HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response);
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
}

// ---- Setup ----
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println();
    Serial.println("=== E-Paper Web Service ===");

    bootTime = millis();

    // Increase watchdog timeout for e-paper refresh (takes ~3.5s)
    esp_task_wdt_deinit();
    esp_task_wdt_init(15, false);

    // Initialize SPI
    SPI.begin(EPD_SCK, -1, EPD_MOSI);
    Serial.println("SPI initialized");

    // Initialize display
    Serial.println("Initializing display...");
    display.init(115200, true, 50, false);
    display.setRotation(0);
    Serial.printf("Display: %dx%d\n", display.width(), display.height());

    // WiFi manager handles connection or AP mode
    bool connected = wifiManagerInit();

    if (connected) {
        Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

        // Sync NTP time
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("NTP time sync requested");

        // mDNS
        if (MDNS.begin("epaper")) {
            Serial.println("mDNS: epaper.local");
            MDNS.addService("http", "tcp", 80);
        }

        // Setup station mode routes and start server
        setupRoutes();
        server.begin();
        Serial.println("HTTP server started on port 80");

        // Show ready screen
        renderReadyScreen();

    } else {
        // AP mode - setup captive portal routes
        Serial.println("AP mode active, starting captive portal");
        setupCaptivePortalRoutes(server);
        server.begin();
        Serial.println("Captive portal server started");
    }
}

// ---- Main loop ----
void loop() {
    // WiFi manager handles DNS processing in AP mode, reconnect in STA mode
    wifiManagerLoop();

    // Countdown timer update (only in station mode)
    if (countdownActive && wifiGetState() == WIFI_STATE_CONNECTED) {
        unsigned long now = millis();
        if (now - lastCountdownUpdate >= 1000 || lastCountdownUpdate == 0) {
            lastCountdownUpdate = now;

            long remaining = 0;
            if (now < countdownTargetMillis) {
                remaining = (long)((countdownTargetMillis - now) / 1000UL);
            }

            Serial.printf("Countdown: %ld seconds remaining\n", remaining);
            renderCountdown(countdownTitle, remaining);

            if (remaining <= 0) {
                countdownActive = false;
            }
        }
    }

    // Clock update (only in station mode)
    if (clockActive && wifiGetState() == WIFI_STATE_CONNECTED) {
        unsigned long now = millis();
        if (now - lastClockUpdate >= 1000 || lastClockUpdate == 0) {
            lastClockUpdate = now;
            renderClock(clockZones, clockZoneCount, clockHour24);
        }
    }

    delay(100);
}
