#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define RSS_MAX_ITEMS 20
#define RSS_MAX_FEEDS 3
#define RSS_BUF_SIZE 8192

struct RssItem {
    char title[128];
    char description[200];
    char feedName[32];
};

static RssItem rssItems[RSS_MAX_ITEMS];
static int rssItemCount = 0;
static char rssFeedUrls[RSS_MAX_FEEDS][256];
static char rssFeedNames[RSS_MAX_FEEDS][32];
static int rssFeedCount = 0;
static int rssDisplayIndex = 0;
static int rssItemsPerPage = 4;
static unsigned long rssLastFetch = 0;
static unsigned long rssFetchInterval = 300000; // 5 min default
static unsigned long rssLastPageFlip = 0;
static unsigned long rssPageFlipInterval = 15000; // 15 sec default
static int rssLastHttpCode = 0;
static int rssLastBytes = 0;
static char rssLastError[64] = "";

// Background fetch state
static volatile bool rssFetchInProgress = false;
static volatile bool rssFetchDone = false;
static int rssFetchResult = 0;  // items fetched

// Extract text content between <tag> and </tag>
// Handles CDATA and strips HTML tags from result
static int rssExtractTag(const char* xml, const char* tag, char* out, int outSize) {
    char openTag[64], closeTag[64];

    // Try CDATA variant first: <tag><![CDATA[...]]></tag>
    snprintf(openTag, sizeof(openTag), "<%s><![CDATA[", tag);
    const char* start = strstr(xml, openTag);
    if (start) {
        start += strlen(openTag);
        const char* end = strstr(start, "]]>");
        if (end) {
            int len = end - start;
            if (len >= outSize) len = outSize - 1;
            memcpy(out, start, len);
            out[len] = '\0';
            goto strip_html;
        }
    }

    // Fall back to simple <tag>...</tag>
    snprintf(openTag, sizeof(openTag), "<%s>", tag);
    snprintf(closeTag, sizeof(closeTag), "</%s>", tag);
    start = strstr(xml, openTag);
    if (!start) {
        out[0] = '\0';
        return 0;
    }

    {
        start += strlen(openTag);
        const char* end = strstr(start, closeTag);
        if (!end) { out[0] = '\0'; return 0; }
        int len = end - start;
        if (len >= outSize) len = outSize - 1;
        memcpy(out, start, len);
        out[len] = '\0';

        // Check if content is CDATA-wrapped (with optional whitespace)
        char* p = out;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (strncmp(p, "<![CDATA[", 9) == 0) {
            char* cdataEnd = strstr(p + 9, "]]>");
            if (cdataEnd) {
                int clen = cdataEnd - (p + 9);
                if (clen >= outSize) clen = outSize - 1;
                memmove(out, p + 9, clen);
                out[clen] = '\0';
            }
        }
    }

strip_html:
    // Strip HTML tags
    {
        char* r = out;
        char* w = out;
        bool inTag = false;
        while (*r) {
            if (*r == '<') { inTag = true; r++; continue; }
            if (*r == '>') { inTag = false; r++; continue; }
            if (!inTag) *w++ = *r;
            r++;
        }
        *w = '\0';
    }

    // Decode common HTML entities
    {
        char* p = out;
        char* w = out;
        while (*p) {
            if (*p == '&') {
                if (strncmp(p, "&amp;", 5) == 0) { *w++ = '&'; p += 5; }
                else if (strncmp(p, "&lt;", 4) == 0) { *w++ = '<'; p += 4; }
                else if (strncmp(p, "&gt;", 4) == 0) { *w++ = '>'; p += 4; }
                else if (strncmp(p, "&quot;", 6) == 0) { *w++ = '"'; p += 6; }
                else if (strncmp(p, "&#39;", 5) == 0) { *w++ = '\''; p += 5; }
                else if (strncmp(p, "&apos;", 6) == 0) { *w++ = '\''; p += 6; }
                else { *w++ = *p++; }
            } else {
                *w++ = *p++;
            }
        }
        *w = '\0';
    }

    return strlen(out);
}

// Fetch a single RSS feed (runs in dedicated task with large stack for TLS)
static bool rssFetchFeed(const char* url, const char* feedName) {
    Serial.printf("RSS: fetching %s, heap=%u\n", url, ESP.getFreeHeap());

    bool isHttps = strncmp(url, "https://", 8) == 0;
    const char* hostStart = url + (isHttps ? 8 : 7);
    const char* pathStart = strchr(hostStart, '/');

    char host[128];
    char path[256];
    int port = isHttps ? 443 : 80;

    if (pathStart) {
        int hostLen = pathStart - hostStart;
        if (hostLen >= (int)sizeof(host)) hostLen = sizeof(host) - 1;
        memcpy(host, hostStart, hostLen);
        host[hostLen] = '\0';
        strncpy(path, pathStart, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {
        strncpy(host, hostStart, sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';
        strcpy(path, "/");
    }

    // Check for port in host
    char* colonPos = strchr(host, ':');
    if (colonPos) {
        port = atoi(colonPos + 1);
        *colonPos = '\0';
    }

    Serial.printf("RSS: %s %s:%d%s\n", isHttps ? "HTTPS" : "HTTP", host, port, path);
    snprintf(rssLastError, sizeof(rssLastError), "connecting %s", host);

    // Use heap-allocated client to avoid stack overflow
    WiFiClient* client;
    if (isHttps) {
        WiFiClientSecure* sc = new WiFiClientSecure();
        if (!sc) {
            snprintf(rssLastError, sizeof(rssLastError), "alloc failed");
            return false;
        }
        sc->setInsecure();
        sc->setTimeout(15);
        sc->setHandshakeTimeout(10);  // 10 seconds max for TLS handshake
        client = sc;
    } else {
        client = new WiFiClient();
        if (!client) {
            snprintf(rssLastError, sizeof(rssLastError), "alloc failed");
            return false;
        }
        client->setTimeout(10);
    }

    if (!client->connect(host, port)) {
        Serial.println("RSS: connect failed");
        snprintf(rssLastError, sizeof(rssLastError), "connect failed %s", host);
        rssLastHttpCode = -1;
        delete client;
        return false;
    }

    snprintf(rssLastError, sizeof(rssLastError), "sending request");
    // Build request as a String and use write() to ensure it goes through TLS
    char reqBuf[512];
    int reqLen = snprintf(reqBuf, sizeof(reqBuf),
        "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: ESP32-EPaper/1.0\r\nAccept: */*\r\nConnection: close\r\n\r\n", path, host);
    client->write((uint8_t*)reqBuf, reqLen);
    client->flush();

    // Read response
    char* buf = (char*)malloc(RSS_BUF_SIZE);
    if (!buf) {
        Serial.println("RSS: malloc failed");
        snprintf(rssLastError, sizeof(rssLastError), "malloc failed");
        client->stop();
        delete client;
        return false;
    }

    int bufLen = 0;
    unsigned long timeout = millis() + 15000;
    bool headersDone = false;
    int httpCode = 0;

    snprintf(rssLastError, sizeof(rssLastError), "reading response");

    while (client->connected() && millis() < timeout) {
        while (client->available() && bufLen < RSS_BUF_SIZE - 1) {
            buf[bufLen++] = client->read();

            // Parse HTTP status code from first line
            if (!headersDone && httpCode == 0 && bufLen > 12) {
                if (strncmp(buf, "HTTP/", 5) == 0) {
                    httpCode = atoi(buf + 9);
                }
            }

            // Detect end of HTTP headers
            if (!headersDone && bufLen >= 4 &&
                buf[bufLen-4] == '\r' && buf[bufLen-3] == '\n' &&
                buf[bufLen-2] == '\r' && buf[bufLen-1] == '\n') {
                headersDone = true;
                bufLen = 0;
            }
        }
        if (bufLen >= RSS_BUF_SIZE - 1) break;
        delay(1);
    }
    while (client->available() && bufLen < RSS_BUF_SIZE - 1) {
        buf[bufLen++] = client->read();
    }
    buf[bufLen] = '\0';

    client->stop();
    delete client;

    rssLastHttpCode = httpCode;
    rssLastBytes = bufLen;
    snprintf(rssLastError, sizeof(rssLastError), "http=%d bytes=%d", httpCode, bufLen);
    Serial.printf("RSS: HTTP %d, %d bytes body\n", httpCode, bufLen);

    if (httpCode != 200 || bufLen == 0) {
        free(buf);
        return false;
    }

    // Parse <item> elements
    char* pos = buf;
    char title[128], desc[200];

    while (rssItemCount < RSS_MAX_ITEMS) {
        char* itemStart = strstr(pos, "<item");
        if (!itemStart) itemStart = strstr(pos, "<entry");
        if (!itemStart) break;

        char* itemEnd = strstr(itemStart, "</item>");
        if (!itemEnd) itemEnd = strstr(itemStart, "</entry>");
        if (!itemEnd) break;

        int endTagLen = (strstr(itemStart, "</entry>") == itemEnd) ? 8 : 7;

        char saved = *(itemEnd + endTagLen);
        *(itemEnd + endTagLen) = '\0';

        rssExtractTag(itemStart, "title", title, sizeof(title));
        rssExtractTag(itemStart, "description", desc, sizeof(desc));
        if (desc[0] == '\0') {
            rssExtractTag(itemStart, "summary", desc, sizeof(desc));
        }

        *(itemEnd + endTagLen) = saved;

        if (title[0]) {
            strncpy(rssItems[rssItemCount].title, title, sizeof(rssItems[0].title) - 1);
            rssItems[rssItemCount].title[sizeof(rssItems[0].title) - 1] = '\0';
            strncpy(rssItems[rssItemCount].description, desc, sizeof(rssItems[0].description) - 1);
            rssItems[rssItemCount].description[sizeof(rssItems[0].description) - 1] = '\0';
            strncpy(rssItems[rssItemCount].feedName, feedName, sizeof(rssItems[0].feedName) - 1);
            rssItems[rssItemCount].feedName[sizeof(rssItems[0].feedName) - 1] = '\0';
            rssItemCount++;
        }

        pos = itemEnd + endTagLen;
    }

    free(buf);
    Serial.printf("RSS: parsed %d total items\n", rssItemCount);
    return true;
}

// Synchronous fetch all feeds (runs in calling context)
void rssFetchAll() {
    int prevCount = rssItemCount;
    rssItemCount = 0;
    rssDisplayIndex = 0;

    for (int i = 0; i < rssFeedCount; i++) {
        rssFetchFeed(rssFeedUrls[i], rssFeedNames[i]);
    }

    rssLastFetch = millis();
    rssFetchDone = true;
    Serial.printf("RSS: refreshed, %d items (was %d)\n", rssItemCount, prevCount);
}

int rssGetPageCount() {
    if (rssItemCount == 0) return 0;
    return (rssItemCount + rssItemsPerPage - 1) / rssItemsPerPage;
}

void rssNextPage() {
    rssDisplayIndex += rssItemsPerPage;
    if (rssDisplayIndex >= rssItemCount) {
        rssDisplayIndex = 0;
    }
}
