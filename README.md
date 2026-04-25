# E-Paper Display Web Service

WiFi-connected e-paper display controller for ESP32-S3 with a 4.2" B/W e-paper panel. Provides a web UI and REST API for pushing content to the display -- text, notifications, dashboards, QR codes, weather, countdowns, clocks, RSS feeds, and images.

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32-S3 DevKitC-1 |
| Display | 4.2" B/W e-paper, 400x300, SSD1683 driver (GDEY042T81) |

### Pin Connections

```
ESP32-S3          E-Paper Display
--------          ---------------
GPIO 11 (MOSI) -> DIN (Data In)
GPIO 12 (SCK)  -> CLK (Clock)
GPIO 10 (CS)   -> CS  (Chip Select)
GPIO  9 (DC)   -> DC  (Data/Command)
GPIO  8 (RST)  -> RST (Reset)
GPIO  7 (BUSY) -> BUSY
3.3V           -> VCC
GND            -> GND
```

> **Note:** MISO is not connected -- e-paper is write-only. SPI is initialized with `SPI.begin(SCK, -1, MOSI)`.

## Features

- **WiFi Manager** -- captive portal for first-time WiFi setup, no hardcoded credentials
- **Web UI** -- dark-themed responsive control panel with tabs for each display mode
- **REST API** -- full HTTP API for automation and integration
- **State persistence** -- display state survives reboots (stored in NVS)
- **Partial refresh** -- flicker-free updates with full refresh on mode switch
- **RSS feeds** -- fetch and rotate through RSS/Atom feeds (HTTPS supported)
- **Big clock** -- 7-segment style digits filling the entire display
- **Image upload** -- any image format (JPG, PNG, GIF, WebP) with browser-side dithering
- **Live weather** -- fetches from Open-Meteo API (no key required)
- **NTP + HTTP fallback** -- time sync works even when UDP port 123 is blocked
- **mDNS** -- accessible at `epaper.local`
- **API key auth** -- optional authentication for all endpoints

## Quick Start

### Build & Flash

```bash
pio run              # Compile
pio run -t upload    # Flash to device
pio device monitor   # Serial output (115200 baud)
```

### First Boot

1. The device starts in AP mode, broadcasting `EPaper-XXXX` (no password)
2. Connect to the AP from your phone or laptop
3. A captive portal opens automatically -- select your WiFi network and enter the password
4. The device reboots, connects to your network, and displays its IP address on the e-paper

### Subsequent Boots

The device reconnects using stored credentials. If connection fails after 15 seconds, it falls back to AP mode.

## Web UI

Open the device IP or `http://epaper.local` in a browser. The UI has tabs for:

| Tab | Description |
|-----|-------------|
| **Text** | Display text with configurable font size, alignment, and position |
| **Notification** | Card, banner, or fullscreen notifications with icon dropdown |
| **Dashboard** | Grid of metric/text widgets (up to 6) |
| **Emoji** | Display pixel art icons fullscreen or normal size with captions |
| **QR Code** | Generate and display QR codes with optional caption |
| **Weather** | Manual or live weather from Open-Meteo (by lat/lon) |
| **Countdown** | Countdown timer to a target date/time (updates every second) |
| **Clock** | Live clock / world clock with up to 4 timezones |
| **Big Clock** | Massive 7-segment digits filling the full display |
| **RSS** | Rotating RSS/Atom feed reader with preset feeds |
| **Image** | Upload any image (JPG/PNG/GIF/WebP) with dithered preview |
| **System** | Device info, display rotation, WiFi reset, API key config |

Each tab shows a collapsible curl command for API automation.

## REST API

All display endpoints accept JSON via POST. Returns `{"ok":true}` on success.

### Authentication

If an API key is configured, include it as a header:

```bash
-H 'X-API-Key: your-key'
```

Set the API key:
```bash
curl -X POST http://epaper.local/api/config \
  -H 'Content-Type: application/json' \
  -d '{"api_key":"your-secret-key"}'
```

---

### Text

Display text with configurable font size, alignment, and position.

```bash
curl -X POST http://epaper.local/api/text \
  -H 'Content-Type: application/json' \
  -d '{"text":"Hello World","size":3,"align":"center"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `text` | string | "Hello" | Text to display |
| `size` | int | 2 | Font size: 1 (9pt), 2 (12pt), 3 (18pt), 4 (24pt) |
| `align` | string | "center" | Alignment: `left`, `center`, `right` |
| `x` | int | 0 | X offset (used with left/right align) |
| `y` | int | 0 | Y offset (0 = auto-center) |

---

### Notification

Card, banner, or fullscreen notification with icon.

```bash
curl -X POST http://epaper.local/api/notification \
  -H 'Content-Type: application/json' \
  -d '{"title":"Alert","body":"Server is down","icon":"warning","style":"card"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `title` | string | "Notification" | Title text |
| `body` | string | "" | Body text |
| `icon` | string | "bell" | Icon name (see Available Icons) |
| `style` | string | "card" | Style: `card`, `banner`, `fullscreen` |

---

### Dashboard

Grid of up to 6 metric/text widgets.

```bash
curl -X POST http://epaper.local/api/dashboard \
  -H 'Content-Type: application/json' \
  -d '{"widgets":[
    {"type":"metric","label":"CPU","value":"42%"},
    {"type":"metric","label":"RAM","value":"8GB"},
    {"type":"text","label":"Status","value":"OK"}
  ]}'
```

| Widget Field | Type | Description |
|-------------|------|-------------|
| `type` | string | `metric` or `text` |
| `label` | string | Widget label |
| `value` | string | Display value |

---

### Emoji

Display pixel art icons at various sizes with optional caption.

```bash
curl -X POST http://epaper.local/api/emoji \
  -H 'Content-Type: application/json' \
  -d '{"emoji":"cat","size":"fullscreen","caption":"MEOW"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `emoji` | string | "cat" | Icon name (see Available Icons) |
| `size` | string | "fullscreen" | Size: `normal`, `fullscreen` |
| `caption` | string | "" | Caption text below icon |

---

### QR Code

Generate and display a QR code.

```bash
curl -X POST http://epaper.local/api/qrcode \
  -H 'Content-Type: application/json' \
  -d '{"data":"https://github.com","caption":"Scan me"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `data` | string | required | Data to encode |
| `caption` | string | "" | Caption text below QR code |

---

### Weather

Display weather manually or fetch live data from Open-Meteo.

**Manual weather:**
```bash
curl -X POST http://epaper.local/api/weather \
  -H 'Content-Type: application/json' \
  -d '{"temp":"22","condition":"sunny","humidity":"45","location":"London"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `temp` | string | required | Temperature |
| `condition` | string | required | One of: `clear`, `sunny`, `cloudy`, `drizzle`, `rainy`, `snowy`, `stormy` |
| `humidity` | string | "" | Humidity percentage |
| `location` | string | "" | Location name |

**Live weather (fetches from Open-Meteo API):**
```bash
curl -X POST http://epaper.local/api/weather/live \
  -H 'Content-Type: application/json' \
  -d '{"lat":51.51,"lon":-0.13,"location":"London"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `lat` | float | required | Latitude |
| `lon` | float | required | Longitude |
| `location` | string | "" | Display name |

---

### Countdown

Countdown timer that updates every second.

```bash
curl -X POST http://epaper.local/api/countdown \
  -H 'Content-Type: application/json' \
  -d '{"title":"Deploy","target":"2026-12-31T00:00:00"}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `title` | string | "Countdown" | Title text |
| `target` | string | required | ISO 8601 datetime: `YYYY-MM-DDTHH:MM:SS` |

---

### Clock / World Clock

Live clock with up to 4 timezones. Updates every second.

```bash
# Single timezone
curl -X POST http://epaper.local/api/clock \
  -H 'Content-Type: application/json' \
  -d '{"hour24":true,"timezones":[{"label":"London","offset":60}]}'

# World clock (up to 4 timezones)
curl -X POST http://epaper.local/api/clock \
  -H 'Content-Type: application/json' \
  -d '{"hour24":false,"timezones":[
    {"label":"London","offset":60},
    {"label":"New York","offset":-240},
    {"label":"Tokyo","offset":540}
  ]}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `hour24` | bool | true | 24-hour format |
| `timezones` | array | [UTC] | Array of timezone objects |
| `timezones[].label` | string | "UTC" | Display name |
| `timezones[].offset` | int | 0 | UTC offset in minutes (e.g. 60 = UTC+1, -300 = UTC-5) |

---

### Big Clock

Full-screen 7-segment clock using the entire 400x300 display. Digits are 230px tall. Updates every second.

```bash
# 24-hour, UTC+1 (London BST)
curl -X POST http://epaper.local/api/bigclock \
  -H 'Content-Type: application/json' \
  -d '{"hour24":true,"offset":60,"label":"London"}'

# 12-hour, UTC-5 (New York EST)
curl -X POST http://epaper.local/api/bigclock \
  -H 'Content-Type: application/json' \
  -d '{"hour24":false,"offset":-300,"label":"New York"}'

# Simple UTC, no label
curl -X POST http://epaper.local/api/bigclock \
  -H 'Content-Type: application/json' \
  -d '{}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `hour24` | bool | true | 24-hour format |
| `offset` | int | 0 | UTC offset in minutes |
| `label` | string | "" | Label shown at bottom (e.g. city name) |

---

### RSS Feed

Fetch and rotate through RSS/Atom feeds. Each story is displayed full-screen with word-wrapped title and description. Updates every 15 seconds, re-fetches every 5 minutes.

```bash
curl -X POST http://epaper.local/api/rss \
  -H 'Content-Type: application/json' \
  -d '{"urls":["https://feeds.bbci.co.uk/news/rss.xml"],
       "names":["BBC News"],
       "items_per_page":4,
       "fetch_interval":300,
       "page_flip_interval":15}'
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `urls` | array | required | Feed URLs (up to 3). HTTP and HTTPS supported |
| `names` | array | [] | Display names for each feed |
| `items_per_page` | int | 4 | Items per page (unused in single-story mode) |
| `fetch_interval` | int | 300 | Seconds between feed re-fetches |
| `page_flip_interval` | int | 15 | Seconds between story rotation |

**Tested feeds:**
- `https://feeds.bbci.co.uk/news/rss.xml` -- BBC News
- `https://feeds.bbci.co.uk/news/technology/rss.xml` -- BBC Tech
- `https://hnrss.org/frontpage` -- Hacker News
- `https://feeds.arstechnica.com/arstechnica/index` -- Ars Technica
- `https://www.theverge.com/rss/index.xml` -- The Verge
- `https://techcrunch.com/feed/` -- TechCrunch
- `https://www.nasa.gov/rss/dyn/breaking_news.rss` -- NASA

---

### Image Upload

Upload any image format (JPG, PNG, GIF, WebP, BMP). The browser converts it to a 400x300 1-bit dithered image before uploading. Images are crop-to-fill (no white bars).

**Via web UI:** Use the Image tab -- select any image file, preview the dithered result, click Upload.

**Via API:** Send raw 1-bit pixel data (15,000 bytes). Each bit represents one pixel (1=black, 0=white), MSB first, top-down scan order, 400 pixels per row.

```bash
# The web UI handles conversion automatically.
# For API usage, convert your image to raw 1-bit 400x300 pixel data first.
curl -X POST http://epaper.local/api/image \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @image.raw
```

| Format | Detail |
|--------|--------|
| Size | Exactly 15,000 bytes (400 x 300 / 8) |
| Bit order | MSB first within each byte |
| Scan order | Top-down, left to right |
| Bit value | 1 = black pixel, 0 = white pixel |

---

### Invert Display

Invert all pixels on the current display.

```bash
curl -X POST http://epaper.local/api/invert
```

---

### Clear Display

Clear to white with a full refresh.

```bash
curl -X POST http://epaper.local/api/clear
```

---

### Device Status

```bash
curl http://epaper.local/api/status
```

Returns JSON:

| Field | Type | Description |
|-------|------|-------------|
| `ip` | string | Device IP address |
| `rssi` | int | WiFi signal strength (dBm) |
| `uptime` | int | Seconds since boot |
| `heap` | int | Free heap memory (bytes) |
| `last_update` | string | Last display mode used |
| `ssid` | string | Connected WiFi network |
| `auth_enabled` | bool | Whether API key is set |
| `epoch` | long | Current Unix timestamp |
| `ntp_synced` | bool | Whether time is synced |
| `rss_active` | bool | RSS feed running |
| `rss_feeds` | int | Number of configured feeds |
| `rss_items` | int | Number of fetched items |
| `rss_last_fetch` | int | Seconds since last fetch (-1 = never) |
| `rss_http` | int | Last HTTP status code |
| `rss_bytes` | int | Bytes received in last fetch |
| `rss_err` | string | Last RSS error/status message |

---

### Configuration

```bash
# Set display rotation (0, 1, 2, 3)
curl -X POST http://epaper.local/api/config \
  -H 'Content-Type: application/json' \
  -d '{"rotation":0}'

# Set API key
curl -X POST http://epaper.local/api/config \
  -H 'Content-Type: application/json' \
  -d '{"api_key":"my-secret-key"}'
```

---

### WiFi Endpoints

```bash
# Scan networks (AP mode only)
curl http://epaper.local/api/wifi/scan

# Save credentials and reboot (AP mode only)
curl -X POST http://epaper.local/api/wifi/connect \
  -H 'Content-Type: application/json' \
  -d '{"ssid":"MyNetwork","password":"secret"}'

# Reset WiFi and reboot into AP mode
curl -X POST http://epaper.local/api/wifi/reset
```

## Available Icons

30 built-in 32x32 pixel art icons:

`warning` `check` `xmark` `heart` `cat` `skull` `fire` `bell` `mail` `clock` `server` `wifi` `battery` `star` `sun` `cloud` `rain` `snow` `storm` `thermometer` `coffee` `rocket` `bug` `shield` `eye` `lightning` `gear` `home` `chart` `user`

Used in: Notification (icon field), Emoji (emoji field), Dashboard headers.

## Common UTC Offsets

| Offset (min) | Timezone |
|-------------|----------|
| -480 | PST (Los Angeles) |
| -420 | MST (Denver) |
| -360 | CST (Chicago) |
| -300 | EST (New York) |
| -240 | AST (Halifax) |
| 0 | GMT/UTC (London winter) |
| 60 | CET/BST (London summer, Paris) |
| 120 | EET (Athens) |
| 180 | MSK (Moscow) |
| 330 | IST (Mumbai) |
| 480 | CST/SGT (Beijing, Singapore) |
| 540 | JST/KST (Tokyo, Seoul) |
| 600 | AEST (Sydney) |
| 720 | NZST (Auckland) |

## Display Refresh

- **Partial refresh** (no flicker) is used for all live modes (clock, countdown, RSS)
- **Full refresh** (black/white flash) triggers automatically when switching between display modes
- The **Clear** button always triggers a full refresh

## Time Sync

1. Tries NTP (`pool.ntp.org`, `time.google.com`, `216.239.35.0`) on boot and retries every 10 seconds
2. After 3 failed NTP attempts, falls back to HTTP Date header parsing from `api.open-meteo.com`
3. This handles networks that block UDP port 123 (NTP)

## Project Structure

```
src/
  main.cpp              # Setup, routes, weather fetch, main loop
  wifi_manager.h        # WiFi state machine, NVS storage, captive portal
  display_renderer.h    # All e-paper rendering (text, clock, big clock, RSS, etc.)
  web_ui.h              # Web UI HTML/CSS/JS
  rss_fetcher.h         # RSS/Atom feed fetcher with HTTPS support
  icons.h               # 32x32 pixel art icon bitmaps
platformio.ini          # PlatformIO build config
```

## Dependencies

Managed by PlatformIO (`platformio.ini`):

- [GxEPD2](https://github.com/ZinggJM/GxEPD2) -- e-paper driver
- [ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer) -- async HTTP server
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) -- JSON parsing
- [QRCode](https://github.com/ricmoo/QRCode) -- QR code generation
- DNSServer, Preferences, ESPmDNS -- built into ESP32 Arduino

## WiFi Manager Flow

```
BOOT --> Check NVS
  |-- Credentials exist --> Try connect (15s timeout)
  |     |-- Success --> STATION MODE (normal operation)
  |     |-- Fail --> AP MODE (fallback)
  |-- No credentials --> AP MODE

AP MODE:
  - SSID: "EPaper-XXXX" (last 4 of MAC, no password)
  - DNS redirects all domains to 192.168.4.1
  - Captive portal serves WiFi setup page
  - E-paper shows AP name + IP instructions

STATION MODE:
  - Web UI + all API endpoints on port 80
  - mDNS at epaper.local
  - System tab has WiFi reset button
```

## Notes

- SPI uses custom pins via explicit `SPI.begin()` -- MISO is -1 (e-paper is write-only)
- Main loop stack increased to 16KB (`ARDUINO_LOOP_STACK_SIZE=16384`) for TLS/HTTPS operations
- RSS HTTPS uses heap-allocated `WiFiClientSecure` with `setInsecure()` and 10-second handshake timeout
- Weather API uses plain HTTP to `api.open-meteo.com` (no API key needed)
- If the display doesn't work with the default driver, try alternatives in GxEPD2 docs for 4.2" panels
