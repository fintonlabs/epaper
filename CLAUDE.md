# E-Paper Display Project

## Hardware
- ESP32-S3 DevKitC-1
- 4.2" B/W e-paper display (400x300, SSD1683 driver)
- Pin mapping: MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO9, RST=GPIO8, BUSY=GPIO7

## Build System
- PlatformIO with Arduino framework
- Library: GxEPD2 (includes Adafruit GFX)

## Commands
```bash
pio run                  # Compile
pio run -t upload        # Flash to device
pio device monitor       # Serial output (115200 baud)
```

## Display Driver
- Default: GxEPD2_420_GDEY042T81 (SSD1683)
- If display doesn't work, try alternatives listed in main.cpp comments
- Common alternatives: GxEPD2_420 (IL0398), GxEPD2_420_M01 (UC8176)

## Notes
- SPI uses custom pins via explicit SPI.begin() - MISO is -1 (e-paper is write-only)
- Device enters deep sleep after display update to save power
- Full refresh used for clean initial display
