# E-Paper Display Web Service

WiFi-connected e-paper display controller for ESP32-S3 with a 4.2" B/W e-paper panel. Provides a web UI and REST API for pushing content to the display -- text, notifications, dashboards, QR codes, weather, countdowns, emoji, and images.

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
- **Partial refresh** -- flicker-free updates with automatic full refresh every 20 cycles
- **mDNS** -- accessible at `epaper.local`
- **NTP** -- time sync for countdown support

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
| **Notification** | Card, banner, or fullscreen notifications with icons |
| **Dashboard** | Grid of metric/text widgets (up to 6) |
| **Emoji** | Display pixel art icons fullscreen or normal size with captions |
| **QR Code** | Generate and display QR codes with optional caption |
| **Weather** | Weather display with temperature, condition, humidity, location |
| **Countdown** | Countdown timer to a target date/time |
| **Image** | Upload and display BMP images (24-bit or 1-bit) |
| **System** | Device info, display rotation, WiFi reset |

Each tab shows a collapsible curl command for API automation.

## REST API

All endpoints accept JSON via POST (except status and scan).

### Display Endpoints

```bash
# Text
curl -X POST http://epaper.local/api/text \
  -H 'Content-Type: application/json' \
  -d '{"text":"Hello World","size":3,"align":"center"}'

# Notification
curl -X POST http://epaper.local/api/notification \
  -H 'Content-Type: application/json' \
  -d '{"title":"Alert","body":"Server is down","icon":"warning","style":"card"}'

# Dashboard (up to 6 widgets)
curl -X POST http://epaper.local/api/dashboard \
  -H 'Content-Type: application/json' \
  -d '{"widgets":[{"type":"metric","label":"CPU","value":"42%"},{"type":"metric","label":"RAM","value":"8GB"}]}'

# Emoji
curl -X POST http://epaper.local/api/emoji \
  -H 'Content-Type: application/json' \
  -d '{"emoji":"cat","size":"fullscreen","caption":"MEOW"}'

# QR Code
curl -X POST http://epaper.local/api/qrcode \
  -H 'Content-Type: application/json' \
  -d '{"data":"https://github.com","caption":"Scan me"}'

# Weather
curl -X POST http://epaper.local/api/weather \
  -H 'Content-Type: application/json' \
  -d '{"temp":"22","condition":"sunny","humidity":"45","location":"London"}'

# Countdown
curl -X POST http://epaper.local/api/countdown \
  -H 'Content-Type: application/json' \
  -d '{"title":"Deploy","target":"2026-12-31T00:00:00"}'

# Image (BMP upload)
curl -X POST http://epaper.local/api/image \
  -F 'image=@photo.bmp'

# Clear display
curl -X POST http://epaper.local/api/clear
```

### Config Endpoints

```bash
# Set display rotation (0, 1, 2, 3)
curl -X POST http://epaper.local/api/config \
  -H 'Content-Type: application/json' \
  -d '{"rotation":0}'

# Device status
curl http://epaper.local/api/status
```

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

`warning` `check` `x-mark` `heart` `cat` `skull` `fire` `bell` `mail` `clock` `server` `wifi` `battery` `star` `sun` `cloud` `rain` `snow` `storm` `thermometer` `coffee` `rocket` `bug` `shield` `eye` `lightning` `gear` `home` `chart` `user`

## Display Refresh

The display uses **partial refresh** by default for flicker-free updates. A full refresh (with the characteristic double-flash) runs automatically every 20 updates to clear accumulated ghosting. The "Clear" button always triggers a full refresh.

## Project Structure

```
src/
  main.cpp              # Setup, routes, QR/image rendering
  wifi_manager.h        # WiFi state machine, NVS storage, captive portal
  display_renderer.h    # All e-paper rendering functions
  web_ui.h              # Main web UI HTML/CSS/JS
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
- If the display doesn't work with the default driver, try alternatives listed in the GxEPD2 documentation for 4.2" panels (IL0398, UC8176)
- Deep sleep support is stubbed but not active -- the device stays awake to serve the web UI
- Image upload supports 24-bit and 1-bit BMP, capped at 120KB
