#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <qrcode.h>
#include <Preferences.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>

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
#include "rss_fetcher.h"
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

// ---- Big Clock state ----
static bool bigClockActive = false;
static int bigClockOffset = 0;
static bool bigClockHour24 = true;
static char bigClockLabel[32] = "";
static unsigned long lastBigClockUpdate = 0;

// ---- RSS state ----
static bool rssActive = false;

// ---- Image upload state (deferred to loop) ----
static uint8_t* imgBuf = nullptr;
static size_t imgSize = 0;
static size_t imgCapacity = 0;
static bool imgRenderPending = false;

// ---- Weather fetch state (deferred to loop) ----
static bool weatherFetchPending = false;
static float weatherLat = 0;
static float weatherLon = 0;
static char weatherLocation[64] = "";

// ---- Config ----
static int displayRotation = 0;

// ---- API Key ----
static char apiKey[64] = "";
static Preferences cfgPrefs;

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

// ---- API Key Authentication ----
void loadApiKey() {
    cfgPrefs.begin("epaper_cfg", true);
    String key = cfgPrefs.getString("api_key", "");
    strncpy(apiKey, key.c_str(), sizeof(apiKey) - 1);
    apiKey[sizeof(apiKey) - 1] = '\0';
    cfgPrefs.end();
    Serial.printf("API key: %s\n", apiKey[0] ? "configured" : "not set (open access)");
}

void saveApiKey(const char* key) {
    cfgPrefs.begin("epaper_cfg", false);
    cfgPrefs.putString("api_key", key);
    cfgPrefs.end();
    strncpy(apiKey, key, sizeof(apiKey) - 1);
    apiKey[sizeof(apiKey) - 1] = '\0';
    Serial.printf("API key: %s\n", key[0] ? "saved" : "cleared");
}

bool checkApiKey(AsyncWebServerRequest* request) {
    // If no key is set, allow all
    if (apiKey[0] == '\0') return true;

    // Check X-API-Key header
    if (request->hasHeader("X-API-Key")) {
        String provided = request->header("X-API-Key");
        if (provided == apiKey) return true;
    }
    return false;
}

void sendUnauthorized(AsyncWebServerRequest* request) {
    request->send(401, "application/json", "{\"ok\":false,\"error\":\"Invalid or missing API key\"}");
}

// ---- Display State Persistence ----
void saveDisplayState(const char* mode, const char* paramsJson) {
    cfgPrefs.begin("epaper_cfg", false);
    cfgPrefs.putString("last_mode", mode);
    cfgPrefs.putString("last_json", paramsJson);
    cfgPrefs.end();
    Serial.printf("State saved: mode=%s\n", mode);
}

bool loadDisplayState(char* mode, size_t modeSize, char* paramsJson, size_t jsonSize) {
    cfgPrefs.begin("epaper_cfg", true);
    String m = cfgPrefs.getString("last_mode", "");
    String j = cfgPrefs.getString("last_json", "");
    cfgPrefs.end();
    if (m.length() == 0) return false;
    strncpy(mode, m.c_str(), modeSize - 1);
    mode[modeSize - 1] = '\0';
    strncpy(paramsJson, j.c_str(), jsonSize - 1);
    paramsJson[jsonSize - 1] = '\0';
    return true;
}

// ---- Restore last display state on boot ----
void restoreDisplayState() {
    char mode[32];
    char params[2048];
    if (!loadDisplayState(mode, sizeof(mode), params, sizeof(params))) {
        Serial.println("No saved state, showing ready screen");
        renderReadyScreen();
        return;
    }

    Serial.printf("Restoring state: mode=%s\n", mode);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, params);
    if (err && strcmp(mode, "clear") != 0) {
        Serial.printf("State restore JSON error: %s\n", err.c_str());
        renderReadyScreen();
        return;
    }

    if (strcmp(mode, "text") == 0) {
        const char* text = doc["text"] | "Hello";
        int size = doc["size"] | 2;
        int x = doc["x"] | 0;
        int y = doc["y"] | 0;
        const char* align = doc["align"] | "center";
        renderText(text, size, x, y, align);

    } else if (strcmp(mode, "notification") == 0) {
        const char* title = doc["title"] | "Notification";
        const char* body = doc["body"] | "";
        const char* icon = doc["icon"] | "bell";
        const char* style = doc["style"] | "card";
        renderNotification(title, body, icon, style);

    } else if (strcmp(mode, "emoji") == 0) {
        const char* emoji = doc["emoji"] | "cat";
        const char* sizeMode = doc["size"] | "fullscreen";
        const char* caption = doc["caption"] | "";
        renderEmoji(emoji, sizeMode, caption);

    } else if (strcmp(mode, "weather") == 0) {
        const char* temp = doc["temp"] | "20";
        const char* cond = doc["condition"] | "sunny";
        const char* hum = doc["humidity"] | "";
        const char* loc = doc["location"] | "";
        renderWeather(temp, cond, hum, loc);

    } else if (strcmp(mode, "countdown") == 0) {
        const char* title = doc["title"] | "Countdown";
        const char* target = doc["target"] | "";
        if (strlen(target) > 0) {
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
                long secondsFromNow = (nowEpoch < 100000) ? 3600 : (long)(targetEpoch - nowEpoch);
                if (secondsFromNow < 0) secondsFromNow = 0;
                countdownTargetMillis = millis() + (unsigned long)secondsFromNow * 1000UL;
                strncpy(countdownTitle, title, sizeof(countdownTitle) - 1);
                countdownTitle[sizeof(countdownTitle) - 1] = '\0';
                countdownActive = true;
                lastCountdownUpdate = 0;
            }
        }

    } else if (strcmp(mode, "clock") == 0) {
        clockHour24 = doc["hour24"] | true;
        JsonArray zones = doc["timezones"].as<JsonArray>();
        clockZoneCount = 0;
        if (!zones.isNull()) {
            for (JsonObject z : zones) {
                if (clockZoneCount >= 4) break;
                strncpy(clockZones[clockZoneCount].label, z["label"] | "UTC",
                        sizeof(clockZones[0].label) - 1);
                clockZones[clockZoneCount].label[sizeof(clockZones[0].label) - 1] = '\0';
                clockZones[clockZoneCount].offsetMinutes = z["offset"] | 0;
                clockZoneCount++;
            }
        }
        if (clockZoneCount == 0) {
            strncpy(clockZones[0].label, "UTC", sizeof(clockZones[0].label));
            clockZones[0].offsetMinutes = 0;
            clockZoneCount = 1;
        }
        clockActive = true;
        lastClockUpdate = 0;

    } else if (strcmp(mode, "bigclock") == 0) {
        bigClockHour24 = doc["hour24"] | true;
        bigClockOffset = doc["offset"] | 0;
        const char* lbl = doc["label"] | "";
        strncpy(bigClockLabel, lbl, sizeof(bigClockLabel) - 1);
        bigClockLabel[sizeof(bigClockLabel) - 1] = '\0';
        bigClockActive = true;
        lastBigClockUpdate = 0;

    } else if (strcmp(mode, "rss") == 0) {
        // Restore RSS config
        rssFeedCount = 0;
        JsonArray urls = doc["urls"].as<JsonArray>();
        if (!urls.isNull()) {
            for (JsonVariant u : urls) {
                if (rssFeedCount >= RSS_MAX_FEEDS) break;
                strncpy(rssFeedUrls[rssFeedCount], u.as<const char*>(), sizeof(rssFeedUrls[0]) - 1);
                rssFeedUrls[rssFeedCount][sizeof(rssFeedUrls[0]) - 1] = '\0';
                snprintf(rssFeedNames[rssFeedCount], sizeof(rssFeedNames[0]), "Feed %d", rssFeedCount + 1);
                rssFeedCount++;
            }
        }
        JsonArray names = doc["names"].as<JsonArray>();
        if (!names.isNull()) {
            for (int i = 0; i < rssFeedCount && i < (int)names.size(); i++) {
                strncpy(rssFeedNames[i], names[i].as<const char*>(), sizeof(rssFeedNames[0]) - 1);
                rssFeedNames[i][sizeof(rssFeedNames[0]) - 1] = '\0';
            }
        }
        rssFetchInterval = doc["interval"] | 300000;
        rssItemsPerPage = doc["items"] | 4;
        rssActive = true;
        rssFetchAll();
        if (rssItemCount > 0) renderRssFeed();

    } else {
        renderReadyScreen();
    }
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

// ---- Weather API fetch (Open-Meteo, free, no key) ----
// Uses plain HTTP to avoid WiFiClientSecure TLS stack overflow in main loop
// Weather codes: https://open-meteo.com/en/docs
static const char* weatherCodeToCondition(int code) {
    if (code == 0) return "clear";
    if (code <= 3) return "cloudy";
    if (code <= 48) return "cloudy";  // fog
    if (code <= 55) return "drizzle";
    if (code <= 65) return "rainy";
    if (code <= 67) return "rainy";   // freezing rain
    if (code <= 75) return "snowy";
    if (code <= 77) return "snowy";   // snow grains
    if (code <= 82) return "rainy";   // showers
    if (code <= 86) return "snowy";   // snow showers
    if (code <= 99) return "stormy";  // thunderstorm
    return "cloudy";
}

bool fetchWeatherFromAPI(float lat, float lon, const char* location,
                         char* outTemp, int tempSize,
                         char* outCond, int condSize,
                         char* outHum, int humSize,
                         char* outLoc, int locSize) {
    WiFiClient client;

    Serial.printf("Weather API: connecting to api.open-meteo.com (HTTP)...\n");
    client.setTimeout(10);  // 10 second timeout

    if (!client.connect("api.open-meteo.com", 80)) {
        Serial.println("Weather API: connect failed");
        return false;
    }

    // Format lat/lon with sign handling for negative values near zero
    char latStr[16], lonStr[16];
    int latSign = (lat < 0) ? -1 : 1;
    float latAbs = lat * latSign;
    int latInt = (int)latAbs;
    int latDec = (int)((latAbs - latInt) * 100);
    snprintf(latStr, sizeof(latStr), "%s%d.%02d", (latSign < 0) ? "-" : "", latInt, latDec);

    int lonSign = (lon < 0) ? -1 : 1;
    float lonAbs = lon * lonSign;
    int lonInt = (int)lonAbs;
    int lonDec = (int)((lonAbs - lonInt) * 100);
    snprintf(lonStr, sizeof(lonStr), "%s%d.%02d", (lonSign < 0) ? "-" : "", lonInt, lonDec);

    char path[256];
    snprintf(path, sizeof(path),
        "/v1/forecast?latitude=%s&longitude=%s"
        "&current=temperature_2m,relative_humidity_2m,weather_code"
        "&temperature_unit=celsius",
        latStr, lonStr);

    Serial.printf("Weather API: GET %s\n", path);
    client.printf("GET %s HTTP/1.0\r\nHost: api.open-meteo.com\r\nConnection: close\r\n\r\n", path);

    // Read response
    char buf[2048];
    int bufLen = 0;
    unsigned long timeout = millis() + 10000;
    bool headersDone = false;

    while (client.connected() && millis() < timeout) {
        while (client.available() && bufLen < (int)sizeof(buf) - 1) {
            buf[bufLen++] = client.read();
            if (!headersDone && bufLen >= 4 &&
                buf[bufLen-4] == '\r' && buf[bufLen-3] == '\n' &&
                buf[bufLen-2] == '\r' && buf[bufLen-1] == '\n') {
                headersDone = true;
                bufLen = 0;
            }
        }
        if (bufLen >= (int)sizeof(buf) - 1) break;
        delay(1);
    }
    while (client.available() && bufLen < (int)sizeof(buf) - 1) {
        buf[bufLen++] = client.read();
    }
    buf[bufLen] = '\0';
    client.stop();

    Serial.printf("Weather API: got %d bytes\n", bufLen);
    if (bufLen < 10) {
        Serial.println("Weather API: response too short");
        return false;
    }

    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf);
    if (err) {
        Serial.printf("Weather API: JSON error: %s\n", err.c_str());
        Serial.printf("Weather API: body='%.100s'\n", buf);
        return false;
    }

    JsonObject current = doc["current"];
    if (current.isNull()) {
        Serial.println("Weather API: no 'current' object");
        return false;
    }

    float temp = current["temperature_2m"] | 0.0f;
    int humidity = current["relative_humidity_2m"] | 0;
    int weatherCode = current["weather_code"] | 0;

    snprintf(outTemp, tempSize, "%.0f", temp);
    strncpy(outCond, weatherCodeToCondition(weatherCode), condSize - 1);
    outCond[condSize - 1] = '\0';
    snprintf(outHum, humSize, "%d", humidity);
    strncpy(outLoc, location, locSize - 1);
    outLoc[locSize - 1] = '\0';

    Serial.printf("Weather API: %.1f C, %s, %d%% humidity\n", temp, outCond, humidity);
    return true;
}

// ---- Stop live modes ----
void stopLiveModes() {
    countdownActive = false;
    clockActive = false;
    bigClockActive = false;
    rssActive = false;
    forceNextFullRefresh = true; // clean slate when switching modes
}

// ---- Setup routes (station mode) ----
void setupRoutes() {
    // Serve web UI
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", WEB_UI_HTML);
    });

    // Status - always open (no auth)
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = (millis() - bootTime) / 1000;
        doc["heap"] = ESP.getFreeHeap();
        doc["last_update"] = lastUpdateType;
        doc["ssid"] = wifiGetSSID();
        doc["auth_enabled"] = (apiKey[0] != '\0');
        doc["epoch"] = (long)time(nullptr);
        doc["ntp_synced"] = time(nullptr) > 100000;
        doc["wx_pending"] = weatherFetchPending;
        doc["wx_lat"] = weatherLat;
        doc["rss_active"] = rssActive;
        doc["rss_feeds"] = rssFeedCount;
        doc["rss_items"] = rssItemCount;
        doc["rss_last_fetch"] = rssLastFetch > 0 ? (long)((millis() - rssLastFetch) / 1000) : -1;
        doc["rss_http"] = rssLastHttpCode;
        doc["rss_bytes"] = rssLastBytes;
        doc["rss_err"] = rssLastError;
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Auth endpoint - set/clear API key
    server.on("/api/auth", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            const char* currentKey = (*doc)["current_key"] | "";
            const char* newKey = (*doc)["new_key"] | "";

            // If key is already set, require current key to change it
            if (apiKey[0] != '\0') {
                if (strcmp(currentKey, apiKey) != 0) {
                    request->send(401, "application/json", "{\"ok\":false,\"error\":\"Current key incorrect\"}");
                    return;
                }
            }

            saveApiKey(newKey);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Text
    server.on("/api/text", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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
            stopLiveModes();
            renderText(txtBuf, size, x, y, alignBuf);
            saveDisplayState("text", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Notification
    server.on("/api/notification", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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
            stopLiveModes();
            renderNotification(nTitle, nBody, nIcon, nStyle);
            saveDisplayState("notification", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Dashboard
    server.on("/api/dashboard", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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

            stopLiveModes();
            renderDashboard(widgets);
            saveDisplayState("dashboard", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Emoji
    server.on("/api/emoji", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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
            stopLiveModes();
            renderEmoji(eName, eSize, eCap);
            saveDisplayState("emoji", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // QR Code
    server.on("/api/qrcode", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }
            static char qrBuf[256], qrCap[128];
            strncpy(qrBuf, (*doc)["data"] | "https://example.com", sizeof(qrBuf)-1); qrBuf[sizeof(qrBuf)-1]='\0';
            strncpy(qrCap, (*doc)["caption"] | "", sizeof(qrCap)-1); qrCap[sizeof(qrCap)-1]='\0';

            Serial.printf("QR: data='%s' caption='%s'\n", qrBuf, qrCap);
            stopLiveModes();
            renderQROnDisplay(qrBuf, qrCap);
            saveDisplayState("qrcode", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Weather
    server.on("/api/weather", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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
            stopLiveModes();
            renderWeather(wTemp, wCond, wHum, wLoc);
            saveDisplayState("weather", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Weather from API (Open-Meteo) - deferred to loop to avoid blocking async handler
    server.on("/api/wx", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            float lat = (*doc)["lat"] | 0.0f;
            float lon = (*doc)["lon"] | 0.0f;

            if (lat == 0.0f && lon == 0.0f) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"lat/lon required\"}");
                return;
            }

            weatherLat = lat;
            weatherLon = lon;
            strncpy(weatherLocation, (*doc)["location"] | "Unknown", sizeof(weatherLocation)-1);
            weatherLocation[sizeof(weatherLocation)-1] = '\0';

            stopLiveModes();
            weatherFetchPending = true;

            request->send(200, "application/json", "{\"ok\":true,\"msg\":\"Fetching weather...\"}");
        }
    );

    // Countdown
    server.on("/api/countdown", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
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

                long secondsFromNow = (nowEpoch < 100000) ? 3600 : (long)(targetEpoch - nowEpoch);
                if (secondsFromNow < 0) secondsFromNow = 0;
                countdownTargetMillis = millis() + (unsigned long)secondsFromNow * 1000UL;
                Serial.printf("Countdown: %ld seconds from now\n", secondsFromNow);
            } else {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid date format\"}");
                return;
            }

            strncpy(countdownTitle, title, sizeof(countdownTitle) - 1);
            countdownTitle[sizeof(countdownTitle) - 1] = '\0';
            stopLiveModes();
            countdownActive = true;
            lastCountdownUpdate = 0;
            saveDisplayState("countdown", jsonBuf);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Clock / World Clock
    server.on("/api/clock", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            clockHour24 = (*doc)["hour24"] | true;
            JsonArray zones = (*doc)["timezones"].as<JsonArray>();
            clockZoneCount = 0;

            if (zones.isNull() || zones.size() == 0) {
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

            stopLiveModes();
            clockActive = true;
            lastClockUpdate = 0;
            saveDisplayState("clock", jsonBuf);

            Serial.printf("Clock: %d zones, 24h=%d\n", clockZoneCount, clockHour24);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Big Clock
    server.on("/api/bigclock", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            bigClockHour24 = (*doc)["hour24"] | true;
            bigClockOffset = (*doc)["offset"] | 0;
            const char* lbl = (*doc)["label"] | "";
            strncpy(bigClockLabel, lbl, sizeof(bigClockLabel) - 1);
            bigClockLabel[sizeof(bigClockLabel) - 1] = '\0';

            stopLiveModes();
            bigClockActive = true;
            lastBigClockUpdate = 0;
            saveDisplayState("bigclock", jsonBuf);

            Serial.printf("BigClock: offset=%d, 24h=%d, label=%s\n", bigClockOffset, bigClockHour24, bigClockLabel);
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // RSS
    server.on("/api/rss", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            JsonArray urls = (*doc)["urls"].as<JsonArray>();
            if (urls.isNull() || urls.size() == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"No feed URLs\"}");
                return;
            }

            rssFeedCount = 0;
            for (JsonVariant u : urls) {
                if (rssFeedCount >= RSS_MAX_FEEDS) break;
                strncpy(rssFeedUrls[rssFeedCount], u.as<const char*>(), sizeof(rssFeedUrls[0]) - 1);
                rssFeedUrls[rssFeedCount][sizeof(rssFeedUrls[0]) - 1] = '\0';
                snprintf(rssFeedNames[rssFeedCount], sizeof(rssFeedNames[0]), "Feed %d", rssFeedCount + 1);
                rssFeedCount++;
            }

            // Optional feed names
            JsonArray names = (*doc)["names"].as<JsonArray>();
            if (!names.isNull()) {
                for (int i = 0; i < rssFeedCount && i < (int)names.size(); i++) {
                    strncpy(rssFeedNames[i], names[i].as<const char*>(), sizeof(rssFeedNames[0]) - 1);
                    rssFeedNames[i][sizeof(rssFeedNames[0]) - 1] = '\0';
                }
            }

            rssFetchInterval = (*doc)["interval"] | 300000;
            rssItemsPerPage = (*doc)["items"] | 4;
            if (rssItemsPerPage < 1) rssItemsPerPage = 1;
            if (rssItemsPerPage > 8) rssItemsPerPage = 8;

            stopLiveModes();
            rssActive = true;

            // Save state for persistence
            saveDisplayState("rss", jsonBuf);

            // Respond immediately, fetch happens in loop
            request->send(200, "application/json", "{\"ok\":true,\"msg\":\"Fetching feeds...\"}");

            // Trigger immediate fetch
            rssLastFetch = 0;
        }
    );

    // Clear
    server.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            stopLiveModes();
            renderClear();
            saveDisplayState("clear", "{}");
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Invert display
    server.on("/api/invert", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            stopLiveModes();
            renderInverted();
            saveDisplayState("invert", "{}");
            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // Image upload
    // Image: raw 1-bit pixel data, 15000 bytes (400x300, 1 bit/pixel, MSB first, top-down)
    // bit=1 means black pixel
    server.on("/api/image", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            const size_t expected = (400 * 300) / 8;  // 15000
            if (index == 0) {
                Serial.printf("Image: receiving %u bytes, heap=%u\n", total, ESP.getFreeHeap());
                if (imgBuf) { free(imgBuf); imgBuf = nullptr; }
                imgBuf = (uint8_t*)malloc(expected);
                imgSize = 0;
            }
            if (imgBuf && imgSize + len <= expected) {
                memcpy(imgBuf + imgSize, data, len);
                imgSize += len;
            }
            if (index + len >= total) {
                Serial.printf("Image: received %u bytes\n", imgSize);
                if (imgBuf && imgSize == expected) {
                    stopLiveModes();
                    imgRenderPending = true;
                    request->send(200, "application/json", "{\"ok\":true}");
                } else {
                    request->send(400, "application/json", "{\"ok\":false,\"error\":\"Expected 15000 bytes\"}");
                    if (imgBuf) { free(imgBuf); imgBuf = nullptr; }
                    imgSize = 0;
                }
            }
        }
    );

    // Config
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            JsonDocument* doc = parseBody(request, data, len);
            if (!doc) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            if ((*doc)["rotation"].is<int>()) {
                displayRotation = (*doc)["rotation"] | 0;
                display.setRotation(displayRotation);
            }

            request->send(200, "application/json", "{\"ok\":true}");
        }
    );

    // WiFi reset
    server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!checkApiKey(request)) { sendUnauthorized(request); return; }
            request->send(200, "application/json", "{\"ok\":true}");
            delay(500);
            wifiClearCredentials();
        }
    );

    // CORS preflight
    server.on("/api/*", HTTP_OPTIONS, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type,X-API-Key");
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

    // Load API key
    loadApiKey();

    // WiFi manager handles connection or AP mode
    bool connected = wifiManagerInit();

    if (connected) {
        Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

        // Sync NTP time
        configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        Serial.print("NTP sync");
        for (int i = 0; i < 30; i++) {  // wait up to 15 seconds
            if (time(nullptr) > 100000) break;
            Serial.print(".");
            delay(500);
        }
        if (time(nullptr) < 100000) {
            // Retry with explicit IP (Google NTP) in case DNS fails
            Serial.print(" retrying with IP...");
            configTime(0, 0, "216.239.35.0", "216.239.35.4");
            for (int i = 0; i < 20; i++) {
                if (time(nullptr) > 100000) break;
                Serial.print(".");
                delay(500);
            }
        }
        Serial.printf(" %s (epoch=%ld)\n",
            time(nullptr) > 100000 ? "OK" : "timeout",
            (long)time(nullptr));

        // mDNS
        if (MDNS.begin("epaper")) {
            Serial.println("mDNS: epaper.local");
            MDNS.addService("http", "tcp", 80);
        }

        // Setup station mode routes and start server
        setupRoutes();
        server.begin();
        Serial.println("HTTP server started on port 80");

        // Restore last display state instead of ready screen
        restoreDisplayState();

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
    wifiManagerLoop();

    if (wifiGetState() != WIFI_STATE_CONNECTED) {
        delay(100);
        return;
    }

    unsigned long now = millis();

    // Background time sync retry if NTP failed
    static unsigned long lastNtpRetry = 0;
    static int ntpAttempt = 0;
    if (time(nullptr) < 100000 && now - lastNtpRetry > 10000) {
        lastNtpRetry = now;
        ntpAttempt++;
        Serial.printf("Time sync attempt %d...\n", ntpAttempt);

        if (ntpAttempt <= 3) {
            // First 3 attempts: try NTP with different servers
            configTime(0, 0, "pool.ntp.org", "time.google.com", "216.239.35.0");
        } else {
            // After NTP fails: use HTTP Date header from any web server
            Serial.println("NTP failed, trying HTTP Date header...");
            WiFiClient client;
            client.setTimeout(10);
            if (client.connect("api.open-meteo.com", 80)) {
                client.print("HEAD / HTTP/1.0\r\nHost: api.open-meteo.com\r\nConnection: close\r\n\r\n");
                client.flush();
                unsigned long httpTimeout = millis() + 8000;
                char buf[512];
                int bufLen = 0;
                while (client.connected() && millis() < httpTimeout && bufLen < (int)sizeof(buf) - 1) {
                    while (client.available() && bufLen < (int)sizeof(buf) - 1) {
                        buf[bufLen++] = client.read();
                    }
                    delay(1);
                }
                buf[bufLen] = '\0';
                client.stop();
                // Parse HTTP Date header: "Date: Fri, 25 Apr 2026 12:34:56 GMT"
                char* dateHdr = strstr(buf, "Date: ");
                if (!dateHdr) dateHdr = strstr(buf, "date: ");
                if (dateHdr) {
                    dateHdr += 6;
                    // Skip day name "Fri, "
                    char* comma = strchr(dateHdr, ',');
                    if (comma) dateHdr = comma + 2;
                    int day, year, hour, min, sec;
                    char mon[4];
                    if (sscanf(dateHdr, "%d %3s %d %d:%d:%d", &day, mon, &year, &hour, &min, &sec) == 6) {
                        const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
                        int monIdx = 0;
                        for (int i = 0; i < 12; i++) { if (strcmp(mon, months[i]) == 0) { monIdx = i; break; } }
                        struct tm tm = {};
                        tm.tm_year = year - 1900;
                        tm.tm_mon = monIdx;
                        tm.tm_mday = day;
                        tm.tm_hour = hour;
                        tm.tm_min = min;
                        tm.tm_sec = sec;
                        time_t epoch = mktime(&tm);
                        if (epoch > 100000) {
                            struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
                            settimeofday(&tv, NULL);
                            Serial.printf("HTTP Date sync OK: epoch=%ld\n", (long)epoch);
                        }
                    } else {
                        Serial.printf("HTTP Date parse failed: %s\n", dateHdr);
                    }
                } else {
                    Serial.printf("No Date header in response (%d bytes)\n", bufLen);
                }
            } else {
                Serial.println("HTTP time: connect failed");
            }
        }
    }

    // Countdown timer update
    if (countdownActive) {
        if (now - lastCountdownUpdate >= 1000 || lastCountdownUpdate == 0) {
            lastCountdownUpdate = now;
            long remaining = 0;
            if (now < countdownTargetMillis) {
                remaining = (long)((countdownTargetMillis - now) / 1000UL);
            }
            renderCountdown(countdownTitle, remaining);
            if (remaining <= 0) countdownActive = false;
        }
    }

    // Clock update
    if (clockActive) {
        if (now - lastClockUpdate >= 1000 || lastClockUpdate == 0) {
            lastClockUpdate = now;
            renderClock(clockZones, clockZoneCount, clockHour24);
        }
    }

    // Big Clock update
    if (bigClockActive) {
        if (now - lastBigClockUpdate >= 1000 || lastBigClockUpdate == 0) {
            lastBigClockUpdate = now;
            renderBigClock(bigClockOffset, bigClockHour24, bigClockLabel);
        }
    }

    // Deferred image render: raw 1-bit data, 15000 bytes, bit=1 is black, MSB first, top-down
    if (imgRenderPending && imgBuf) {
        imgRenderPending = false;
        Serial.println("Image: rendering to display");
        setDisplayWindow(true);
        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            for (int y = 0; y < 300; y++) {
                for (int x = 0; x < 400; x++) {
                    int bitIndex = y * 400 + x;
                    if (imgBuf[bitIndex >> 3] & (0x80 >> (bitIndex & 7))) {
                        display.drawPixel(x, y, GxEPD_BLACK);
                    }
                }
            }
        } while (display.nextPage());
        setLastUpdate("image");
        saveDisplayState("image", "{}");
        free(imgBuf); imgBuf = nullptr; imgSize = 0;
    }

    // Deferred weather fetch (runs in loop to avoid blocking async handler)
    if (weatherFetchPending) {
        weatherFetchPending = false;
        Serial.println("Weather: fetching from Open-Meteo...");

        static char wTemp[16], wCond[32], wHum[16], wLoc[64];
        strncpy(wLoc, weatherLocation, sizeof(wLoc) - 1);
        wLoc[sizeof(wLoc) - 1] = '\0';

        bool ok = fetchWeatherFromAPI(weatherLat, weatherLon, weatherLocation,
                wTemp, sizeof(wTemp), wCond, sizeof(wCond),
                wHum, sizeof(wHum), wLoc, sizeof(wLoc));
        Serial.printf("Weather fetch result: %s\n", ok ? "OK" : "FAILED");
        if (ok) {
            renderWeather(wTemp, wCond, wHum, wLoc);

            char stateJson[256];
            snprintf(stateJson, sizeof(stateJson),
                "{\"temp\":\"%s\",\"condition\":\"%s\",\"humidity\":\"%s\",\"location\":\"%s\",\"lat\":%.2f,\"lon\":%.2f}",
                wTemp, wCond, wHum, wLoc, weatherLat, weatherLon);
            saveDisplayState("weather", stateJson);
            Serial.printf("Weather: %s, %s, %s%% @ %s\n", wTemp, wCond, wHum, wLoc);
        } else {
            renderText("Weather fetch failed", 2, 0, 0, "center");
            Serial.println("Weather: fetch failed, displayed error");
        }
    }

    // RSS: check if fetch just completed
    if (rssFetchDone) {
        rssFetchDone = false;
        if (rssActive && rssItemCount > 0) {
            renderRssFeed();
            rssLastPageFlip = now;
        }
    }

    // RSS feed management
    if (rssActive && rssFeedCount > 0) {
        // Periodic fetch
        if (now - rssLastFetch >= rssFetchInterval || rssLastFetch == 0) {
            rssFetchAll();
        }
        // Rotate to next story
        else if (rssItemCount > 1 && now - rssLastPageFlip >= rssPageFlipInterval) {
            rssLastPageFlip = now;
            rssDisplayIndex++;
            if (rssDisplayIndex >= rssItemCount) rssDisplayIndex = 0;
            renderRssFeed();
        }
    }

    delay(100);
}
