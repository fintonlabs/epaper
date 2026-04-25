#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// WiFi manager state
enum WiFiManagerState {
    WIFI_STATE_INIT,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
};

static WiFiManagerState wifiState = WIFI_STATE_INIT;
static DNSServer dnsServer;
static Preferences preferences;
static char apSSID[32] = "";
static char storedSSID[64] = "";
static char storedPass[64] = "";
static bool wifiConfigured = false;

// Forward declarations for display functions (defined in display_renderer.h)
void renderAPSetupScreen(const char* apName, const char* ipAddr);
void renderWiFiConnecting(const char* ssid);

// Generate AP name from MAC address
void wifiGenerateAPName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(apSSID, sizeof(apSSID), "EPaper-%02X%02X", mac[4], mac[5]);
}

// Read stored credentials from NVS
bool wifiLoadCredentials() {
    preferences.begin("epaper", true); // read-only
    wifiConfigured = preferences.getBool("configured", false);
    if (wifiConfigured) {
        String ssid = preferences.getString("wifi_ssid", "");
        String pass = preferences.getString("wifi_pass", "");
        strncpy(storedSSID, ssid.c_str(), sizeof(storedSSID) - 1);
        storedSSID[sizeof(storedSSID) - 1] = '\0';
        strncpy(storedPass, pass.c_str(), sizeof(storedPass) - 1);
        storedPass[sizeof(storedPass) - 1] = '\0';
    }
    preferences.end();
    Serial.printf("NVS: configured=%d ssid='%s'\n", wifiConfigured, storedSSID);
    return wifiConfigured;
}

// Save credentials to NVS
void wifiSaveCredentials(const char* ssid, const char* pass) {
    preferences.begin("epaper", false); // read-write
    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", pass);
    preferences.putBool("configured", true);
    preferences.end();
    Serial.printf("NVS: saved ssid='%s'\n", ssid);
}

// Clear credentials and reboot
void wifiClearCredentials() {
    preferences.begin("epaper", false);
    preferences.clear();
    preferences.end();
    Serial.println("NVS: cleared credentials, rebooting...");
    delay(500);
    ESP.restart();
}

// Try connecting to WiFi with timeout
bool wifiTryConnect(const char* ssid, const char* pass, unsigned long timeoutMs) {
    Serial.printf("WiFi: connecting to '%s'...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi: connected, IP=%s\n", WiFi.localIP().toString().c_str());
        wifiState = WIFI_STATE_CONNECTED;
        return true;
    }

    Serial.println("WiFi: connection failed");
    return false;
}

// Start AP mode with captive portal
void wifiStartAP() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID);
    delay(100); // Let AP stabilize

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("WiFi: AP mode, SSID='%s', IP=%s\n", apSSID, apIP.toString().c_str());

    // Start DNS server to redirect all domains to our IP (captive portal)
    dnsServer.start(53, "*", apIP);

    wifiState = WIFI_STATE_AP_MODE;

    // Show AP setup screen on e-paper
    renderAPSetupScreen(apSSID, apIP.toString().c_str());
}

// WiFi scan endpoint handler - returns JSON array of networks
void handleWiFiScan(AsyncWebServerRequest* request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true); // async scan
        request->send(202, "application/json", "{\"scanning\":true}");
        return;
    }
    if (n == WIFI_SCAN_RUNNING) {
        request->send(202, "application/json", "{\"scanning\":true}");
        return;
    }

    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    // Deduplicate by SSID, keep strongest signal
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;

        bool found = false;
        for (JsonObject net : networks) {
            if (net["ssid"].as<String>() == ssid) {
                if (WiFi.RSSI(i) > net["rssi"].as<int>()) {
                    net["rssi"] = WiFi.RSSI(i);
                    net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            JsonObject net = networks.add<JsonObject>();
            net["ssid"] = ssid;
            net["rssi"] = WiFi.RSSI(i);
            net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
    }

    WiFi.scanDelete();
    WiFi.scanNetworks(true); // Start new scan for next request

    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out);
}

// Captive portal HTML page
const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>E-Paper WiFi Setup</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{background:#111;color:#e0e0e0;font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:20px}
.card{background:#1a1a1a;border:1px solid #333;border-radius:16px;padding:28px;max-width:400px;width:100%}
h1{font-size:22px;text-align:center;margin-bottom:4px}
.subtitle{text-align:center;color:#888;font-size:13px;margin-bottom:24px}
.net-list{max-height:260px;overflow-y:auto;margin-bottom:20px}
.net-item{display:flex;align-items:center;justify-content:space-between;padding:12px 14px;background:#222;border:1px solid #333;border-radius:10px;margin-bottom:6px;cursor:pointer;transition:all .15s}
.net-item:hover{background:#2a2a2a;border-color:#555}
.net-item.selected{border-color:#2563eb;background:#1e293b}
.net-name{font-size:14px;font-weight:500}
.net-meta{display:flex;align-items:center;gap:8px;font-size:12px;color:#888}
.signal{display:flex;gap:1px;align-items:flex-end;height:14px}
.signal span{width:3px;background:#555;border-radius:1px}
.signal span.active{background:#4ade80}
.lock{font-size:10px}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:13px;color:#888;margin-bottom:6px}
.form-group input{width:100%;padding:12px 14px;background:#222;border:1px solid #444;border-radius:10px;color:#e0e0e0;font-size:14px;outline:none}
.form-group input:focus{border-color:#2563eb}
.btn{width:100%;padding:14px;background:#2563eb;color:#fff;border:none;border-radius:10px;cursor:pointer;font-size:15px;font-weight:600;transition:all .15s}
.btn:hover{background:#1d4ed8}
.btn:disabled{background:#333;color:#666;cursor:not-allowed}
.btn .spinner{display:inline-block;width:16px;height:16px;border:2px solid #fff4;border-top-color:#fff;border-radius:50%;animation:spin .6s linear infinite;margin-right:8px;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
.status{text-align:center;margin-top:14px;font-size:13px;color:#888;min-height:20px}
.status.error{color:#f87171}
.status.success{color:#4ade80}
.scan-btn{background:none;border:1px solid #444;color:#888;padding:6px 14px;border-radius:6px;cursor:pointer;font-size:12px;margin-bottom:12px}
.scan-btn:hover{border-color:#666;color:#ccc}
.empty{text-align:center;padding:24px;color:#666;font-size:13px}
</style>
</head>
<body>
<div class="card">
  <h1>WiFi Setup</h1>
  <p class="subtitle">Connect your E-Paper display to WiFi</p>

  <button class="scan-btn" onclick="scanNetworks()">Refresh Networks</button>
  <div class="net-list" id="net-list">
    <div class="empty">Scanning for networks...</div>
  </div>

  <div class="form-group" id="ssid-group" style="display:none">
    <label>Network Name</label>
    <input type="text" id="ssid" placeholder="SSID" readonly>
  </div>
  <div class="form-group" id="pass-group" style="display:none">
    <label>Password</label>
    <input type="password" id="password" placeholder="Enter password">
  </div>

  <button class="btn" id="connect-btn" onclick="connectWiFi()" disabled>Select a Network</button>
  <div class="status" id="status"></div>
</div>

<script>
let selectedSSID = '';
let selectedSecure = false;

function signalBars(rssi) {
  const bars = rssi > -50 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  let html = '<div class="signal">';
  for (let i = 1; i <= 4; i++) {
    html += '<span style="height:' + (i * 3 + 2) + 'px" class="' + (i <= bars ? 'active' : '') + '"></span>';
  }
  return html + '</div>';
}

async function scanNetworks() {
  document.getElementById('net-list').innerHTML = '<div class="empty">Scanning...</div>';
  try {
    const r = await fetch('/api/wifi/scan');
    const d = await r.json();
    if (d.scanning) {
      setTimeout(scanNetworks, 2000);
      return;
    }
    const nets = d.networks || [];
    if (nets.length === 0) {
      document.getElementById('net-list').innerHTML = '<div class="empty">No networks found. Try again.</div>';
      return;
    }
    nets.sort((a, b) => b.rssi - a.rssi);
    document.getElementById('net-list').innerHTML = nets.map(n =>
      '<div class="net-item" onclick="selectNetwork(\'' + n.ssid.replace(/'/g, "\\'") + '\',' + n.secure + ')">' +
        '<span class="net-name">' + n.ssid + '</span>' +
        '<div class="net-meta">' + (n.secure ? '<span class="lock">&#x1F512;</span>' : '') + signalBars(n.rssi) + '</div>' +
      '</div>'
    ).join('');
  } catch(e) {
    document.getElementById('net-list').innerHTML = '<div class="empty">Scan failed. Try again.</div>';
  }
}

function selectNetwork(ssid, secure) {
  selectedSSID = ssid;
  selectedSecure = secure;
  document.querySelectorAll('.net-item').forEach(el => el.classList.remove('selected'));
  event.currentTarget.classList.add('selected');
  document.getElementById('ssid').value = ssid;
  document.getElementById('ssid-group').style.display = 'block';
  if (secure) {
    document.getElementById('pass-group').style.display = 'block';
    document.getElementById('password').focus();
  } else {
    document.getElementById('pass-group').style.display = 'none';
  }
  const btn = document.getElementById('connect-btn');
  btn.disabled = false;
  btn.textContent = 'Connect to ' + ssid;
}

async function connectWiFi() {
  const ssid = selectedSSID;
  const password = document.getElementById('password').value;
  const btn = document.getElementById('connect-btn');
  const status = document.getElementById('status');

  btn.disabled = true;
  btn.innerHTML = '<span class="spinner"></span>Connecting...';
  status.className = 'status';
  status.textContent = 'Saving credentials and restarting...';

  try {
    const r = await fetch('/api/wifi/connect', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({ssid, password})
    });
    const d = await r.json();
    if (d.ok) {
      status.className = 'status success';
      status.textContent = 'Credentials saved. Device is restarting...';
    } else {
      status.className = 'status error';
      status.textContent = d.error || 'Failed to save';
      btn.disabled = false;
      btn.textContent = 'Connect to ' + ssid;
    }
  } catch(e) {
    status.className = 'status success';
    status.textContent = 'Device is restarting. Connect to your WiFi network.';
  }
}

// Initial scan
scanNetworks();
</script>
</body>
</html>
)rawliteral";

// Setup captive portal routes on the web server
void setupCaptivePortalRoutes(AsyncWebServer& server) {
    // Serve captive portal page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", CAPTIVE_PORTAL_HTML);
    });

    // Captive portal detection endpoints - redirect to portal
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "success");
    });

    // WiFi scan
    server.on("/api/wifi/scan", HTTP_GET, handleWiFiScan);

    // WiFi connect - save credentials and reboot
    server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            if (len > 512) len = 512;
            char buf[513];
            memcpy(buf, data, len);
            buf[len] = '\0';

            DeserializationError err = deserializeJson(doc, buf);
            if (err) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            const char* ssid = doc["ssid"] | "";
            const char* password = doc["password"] | "";

            if (strlen(ssid) == 0) {
                request->send(400, "application/json", "{\"ok\":false,\"error\":\"SSID required\"}");
                return;
            }

            wifiSaveCredentials(ssid, password);
            request->send(200, "application/json", "{\"ok\":true}");

            // Reboot after a short delay to let the response send
            delay(1000);
            ESP.restart();
        }
    );

    // Catch-all for captive portal - redirect unknown paths to portal
    server.onNotFound([](AsyncWebServerRequest* request) {
        request->redirect("/");
    });

    // Start WiFi scan immediately
    WiFi.scanNetworks(true);
}

// Initialize WiFi manager - call from setup()
// Returns true if connected to station mode, false if in AP mode
bool wifiManagerInit() {
    wifiGenerateAPName();
    wifiLoadCredentials();

    if (wifiConfigured && strlen(storedSSID) > 0) {
        wifiState = WIFI_STATE_CONNECTING;
        renderWiFiConnecting(storedSSID);

        if (wifiTryConnect(storedSSID, storedPass, 15000)) {
            return true;
        }
        // Connection failed - fall through to AP mode
        Serial.println("WiFi: stored credentials failed, entering AP mode");
    }

    wifiStartAP();
    return false;
}

// Call from loop() - handles DNS in AP mode, reconnect in STA mode
void wifiManagerLoop() {
    if (wifiState == WIFI_STATE_AP_MODE) {
        dnsServer.processNextRequest();
    } else if (wifiState == WIFI_STATE_CONNECTED) {
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 30000) {
            lastCheck = millis();
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("WiFi: lost connection, reconnecting...");
                WiFi.reconnect();
            }
        }
    }
}

// Get current state
WiFiManagerState wifiGetState() {
    return wifiState;
}

// Get AP SSID
const char* wifiGetAPName() {
    return apSSID;
}

// Get stored SSID (for display in connected mode)
const char* wifiGetSSID() {
    if (wifiState == WIFI_STATE_CONNECTED) {
        return storedSSID;
    }
    return apSSID;
}
