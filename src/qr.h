#pragma once
#include <Arduino.h>
#include <qrcode.h>

// QR code generation wrapper
// Returns a dynamically allocated QRCode struct. Caller must free modules buffer.

struct QRResult {
    uint8_t* modules;
    uint8_t size;
    bool valid;
};

QRResult generateQR(const char* data) {
    QRResult result = {nullptr, 0, false};

    // Use version 6 (41x41) for decent capacity
    int version = 3; // 29x29, ~77 alphanumeric chars
    int len = strlen(data);
    if (len > 77) version = 5; // 37x37
    if (len > 134) version = 8; // 49x49

    QRCode qrcode;
    int bufSize = qrcode_getBufferSize(version);
    uint8_t* qrcodeData = (uint8_t*)malloc(bufSize);
    if (!qrcodeData) return result;

    int err = qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, data);
    if (err != 0) {
        free(qrcodeData);
        return result;
    }

    result.modules = qrcodeData;
    result.size = qrcode.size;
    result.valid = true;

    // Store QRCode struct data we need
    // We'll re-read using qrcode_getModule
    return result;
}

// Draw QR code on display at position with given pixel scale
// Requires access to the display object and the QRCode struct
void drawQRCode(const char* data, int16_t startX, int16_t startY, int pixelSize,
                GxEPD2_BW<GxEPD2_420_M01, GxEPD2_420_M01::HEIGHT>& display) {
    int version = 3;
    int len = strlen(data);
    if (len > 77) version = 5;
    if (len > 134) version = 8;

    QRCode qrcode;
    int bufSize = qrcode_getBufferSize(version);
    uint8_t* qrcodeData = (uint8_t*)malloc(bufSize);
    if (!qrcodeData) return;

    int err = qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, data);
    if (err != 0) {
        free(qrcodeData);
        return;
    }

    // Draw white border
    int totalSize = qrcode.size * pixelSize + 8;
    display.fillRect(startX - 4, startY - 4, totalSize, totalSize, GxEPD_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                display.fillRect(startX + x * pixelSize, startY + y * pixelSize,
                                 pixelSize, pixelSize, GxEPD_BLACK);
            }
        }
    }

    free(qrcodeData);
}
