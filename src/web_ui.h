#pragma once

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>E-Paper Display</title>
<style>
:root{
  --bg:#111;--bg2:#1a1a1a;--bg3:#222;--border:#333;--border2:#444;
  --text:#e0e0e0;--text2:#888;--text3:#999;--accent:#2563eb;--accent2:#1d4ed8;
  --green:#4ade80;--red:#dc2626;--red2:#b91c1c;
}
*{margin:0;padding:0;box-sizing:border-box}
body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;font-size:13px;min-height:100vh}

/* Header - compact */
.header{background:var(--bg2);border-bottom:1px solid var(--border);padding:8px 16px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:6px}
.header-left{display:flex;align-items:center;gap:8px}
.header h1{font-size:15px;font-weight:600;color:#fff}
.pulse{width:7px;height:7px;border-radius:50%;background:var(--green);box-shadow:0 0 5px var(--green);animation:pulse-anim 2s infinite}
@keyframes pulse-anim{0%,100%{opacity:1}50%{opacity:.4}}
.status-bar{display:flex;gap:10px;font-size:11px;color:var(--text2);flex-wrap:wrap}
.status-bar .val{color:var(--green)}

/* Container */
.container{max-width:860px;margin:0 auto;padding:12px}

/* Tabs - compact pills */
.tabs{display:flex;gap:4px;margin-bottom:12px;flex-wrap:wrap;overflow-x:auto;-webkit-overflow-scrolling:touch;scrollbar-width:none}
.tabs::-webkit-scrollbar{display:none}
.tab{padding:6px 12px;background:var(--bg2);border:1px solid var(--border);border-radius:16px;cursor:pointer;font-size:12px;color:var(--text3);transition:all .2s;white-space:nowrap;user-select:none}
.tab:hover{background:var(--bg3);color:#ccc}
.tab.active{background:var(--accent);border-color:var(--accent);color:#fff}
.tab[data-tab="system"]{border-color:#555}
.tab[data-tab="system"].active{background:#6366f1;border-color:#6366f1}

/* Panel */
.panel{display:none;background:var(--bg2);border:1px solid var(--border);border-radius:10px;padding:16px}
.panel.active{display:block}

/* Forms - compact */
.form-group{margin-bottom:10px}
.form-group label{display:block;font-size:11px;color:var(--text2);margin-bottom:3px;font-weight:500;text-transform:uppercase;letter-spacing:.4px}
.form-group input,.form-group textarea,.form-group select{width:100%;padding:8px 10px;background:var(--bg3);border:1px solid var(--border2);border-radius:6px;color:var(--text);font-size:13px;outline:none;transition:border .2s}
.form-group input:focus,.form-group textarea:focus,.form-group select:focus{border-color:var(--accent)}
.form-group textarea{resize:vertical;min-height:56px}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.form-row-3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}

/* Buttons - compact */
.btn{padding:8px 16px;background:var(--accent);color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:12px;font-weight:600;transition:all .15s;display:inline-flex;align-items:center;gap:5px}
.btn:hover{background:var(--accent2)}
.btn:active{transform:scale(0.98)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.btn-secondary{background:var(--border);color:#ccc}
.btn-secondary:hover{background:var(--border2)}
.btn-danger{background:var(--red)}
.btn-danger:hover{background:var(--red2)}
.btn .spinner{display:inline-block;width:12px;height:12px;border:2px solid #fff4;border-top-color:#fff;border-radius:50%;animation:spin .6s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.actions{display:flex;gap:6px;margin-top:12px;flex-wrap:wrap}

/* Curl box */
.curl-toggle{margin-top:8px;background:none;border:none;color:var(--text2);cursor:pointer;font-size:11px;padding:2px 0;display:flex;align-items:center;gap:4px}
.curl-toggle:hover{color:var(--text)}
.curl-box{margin-top:4px;background:#0a0a0a;border:1px solid var(--border);border-radius:6px;padding:10px;font-family:'Courier New',monospace;font-size:10px;color:var(--green);word-break:break-all;position:relative;display:none}
.curl-box.show{display:block}
.curl-box .copy-btn{position:absolute;top:4px;right:4px;background:var(--border);color:#ccc;border:none;padding:2px 6px;border-radius:3px;cursor:pointer;font-size:10px}

/* Toast */
.toast-container{position:fixed;top:12px;right:12px;z-index:1000;display:flex;flex-direction:column;gap:6px}
.toast{padding:10px 14px;border-radius:8px;font-size:12px;color:#fff;animation:toast-in .3s;max-width:280px;box-shadow:0 4px 12px #0008}
.toast.success{background:#166534;border:1px solid #22c55e}
.toast.error{background:#7f1d1d;border:1px solid #f87171}
@keyframes toast-in{from{opacity:0;transform:translateX(40px)}to{opacity:1;transform:none}}

/* Icon grid - compact */
.icon-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(64px,1fr));gap:4px;margin-top:4px}
.icon-opt{padding:5px;background:var(--bg3);border:2px solid var(--border);border-radius:6px;cursor:pointer;text-align:center;font-size:10px;color:var(--text2);transition:all .15s}
.icon-opt:hover{border-color:#555;color:#ccc}
.icon-opt.selected{border-color:var(--accent);color:#fff;background:#1e3a5f}

/* Widget list */
.widget-list{margin-top:8px}
.widget-item{background:var(--bg3);border:1px solid var(--border);border-radius:6px;padding:8px;margin-bottom:4px;display:grid;grid-template-columns:1fr 1fr 1fr auto;gap:6px;align-items:center}
.widget-item input,.widget-item select{padding:5px 6px;background:var(--bg2);border:1px solid var(--border2);border-radius:4px;color:var(--text);font-size:12px}
.widget-item .remove-btn{background:var(--red);color:#fff;border:none;padding:3px 7px;border-radius:4px;cursor:pointer;font-size:14px}

/* System panel */
.sys-section{margin-bottom:16px}
.sys-section h3{font-size:12px;color:var(--text2);margin-bottom:8px;text-transform:uppercase;letter-spacing:.4px}
.sys-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}
.sys-item{background:var(--bg3);border:1px solid var(--border);border-radius:6px;padding:10px}
.sys-item .label{font-size:10px;color:var(--text2);text-transform:uppercase;letter-spacing:.4px;margin-bottom:2px}
.sys-item .value{font-size:13px;color:var(--green);font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.danger-zone{border:1px solid #7f1d1d;border-radius:6px;padding:12px;background:#1a0000}
.danger-zone p{font-size:12px;color:var(--text2);margin-bottom:8px}

/* Auth section */
.auth-section{border:1px solid var(--border);border-radius:6px;padding:12px;background:var(--bg3);margin-bottom:16px}
.auth-section h3{font-size:12px;color:var(--text2);margin-bottom:8px;text-transform:uppercase;letter-spacing:.4px}
.auth-status{font-size:12px;margin-bottom:8px;padding:6px 10px;border-radius:4px}
.auth-status.locked{background:#1e293b;color:#60a5fa;border:1px solid #2563eb44}
.auth-status.open{background:#1a2e1a;color:var(--green);border:1px solid #22c55e44}

/* RSS feed list */
.rss-feed-list{margin-top:8px}
.rss-feed-item{background:var(--bg3);border:1px solid var(--border);border-radius:6px;padding:8px;margin-bottom:4px;display:grid;grid-template-columns:80px 1fr auto;gap:6px;align-items:center}
.rss-feed-item input{padding:5px 6px;background:var(--bg2);border:1px solid var(--border2);border-radius:4px;color:var(--text);font-size:12px}

/* History */
.history{margin-top:14px}
.history h3{font-size:11px;color:var(--text2);margin-bottom:6px;text-transform:uppercase;letter-spacing:.4px}
.history-item{padding:5px 8px;background:var(--bg2);border:1px solid var(--bg3);border-radius:4px;margin-bottom:2px;font-size:11px;color:var(--text3);display:flex;justify-content:space-between}
.history-item .type{color:var(--green);font-weight:600}

/* Mobile responsive */
@media(max-width:600px){
  .header{padding:8px 10px}
  .header h1{font-size:14px}
  .status-bar{font-size:10px;gap:6px}
  .container{padding:8px}
  .panel{padding:12px}
  .form-row,.form-row-3{grid-template-columns:1fr}
  .sys-grid{grid-template-columns:1fr 1fr}
  .widget-item{grid-template-columns:1fr 1fr;gap:4px}
  .rss-feed-item{grid-template-columns:1fr;gap:4px}
  .btn{padding:10px 14px;min-height:40px;font-size:13px}
  .tab{padding:8px 12px;min-height:40px;display:flex;align-items:center}
}
</style>
</head>
<body>
<div class="header">
  <div class="header-left">
    <div class="pulse"></div>
    <h1>E-Paper Display</h1>
    <span style="font-size:11px;color:#888" id="st-ssid"></span>
  </div>
  <div class="status-bar">
    <span>IP: <span class="val" id="st-ip">--</span></span>
    <span>RSSI: <span class="val" id="st-rssi">--</span></span>
    <span>Up: <span class="val" id="st-uptime">--</span></span>
    <span>Heap: <span class="val" id="st-heap">--</span></span>
  </div>
</div>
<div class="toast-container" id="toast-container"></div>
<div class="container">
  <div class="tabs">
    <div class="tab active" data-tab="text">Text</div>
    <div class="tab" data-tab="notification">Notification</div>
    <div class="tab" data-tab="dashboard">Dashboard</div>
    <div class="tab" data-tab="emoji">Emoji</div>
    <div class="tab" data-tab="qrcode">QR Code</div>
    <div class="tab" data-tab="weather">Weather</div>
    <div class="tab" data-tab="countdown">Countdown</div>
    <div class="tab" data-tab="clock">Clock</div>
    <div class="tab" data-tab="bigclock">Big Clock</div>
    <div class="tab" data-tab="rss">RSS</div>
    <div class="tab" data-tab="image">Image</div>
    <div class="tab" data-tab="system">System</div>
  </div>

  <!-- TEXT -->
  <div class="panel active" id="panel-text">
    <div class="form-group"><label>Text</label><textarea id="txt-text" placeholder="Hello World" rows="2">Hello World</textarea></div>
    <div class="form-row">
      <div class="form-group"><label>Font Size (1-4)</label><input type="number" id="txt-size" value="3" min="1" max="4"></div>
      <div class="form-group"><label>Alignment</label>
        <select id="txt-align"><option value="center">Center</option><option value="left">Left</option><option value="right">Right</option></select>
      </div>
    </div>
    <div class="form-row">
      <div class="form-group"><label>X Position (0=auto)</label><input type="number" id="txt-x" value="0"></div>
      <div class="form-group"><label>Y Position (0=auto)</label><input type="number" id="txt-y" value="0"></div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-text" onclick="sendText()">Display Text</button>
      <button class="btn btn-secondary" onclick="clearDisplay()">Clear</button>
      <button class="btn btn-secondary" onclick="invertDisplay()">Invert</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('text')">&#9654; curl command</button>
    <div class="curl-box" id="curl-text"></div>
  </div>

  <!-- NOTIFICATION -->
  <div class="panel" id="panel-notification">
    <div class="form-group"><label>Title</label><input type="text" id="notif-title" placeholder="Deploy Failed" value="Alert"></div>
    <div class="form-group"><label>Body</label><textarea id="notif-body" placeholder="Build #482 crashed on staging" rows="2">Something happened</textarea></div>
    <div class="form-row">
      <div class="form-group"><label>Style</label>
        <select id="notif-style"><option value="card">Card</option><option value="banner">Banner</option><option value="fullscreen">Fullscreen</option></select>
      </div>
      <div class="form-group"><label>Icon</label>
        <select id="notif-icon">
          <option value="warning">warning</option>
          <option value="bell">bell</option>
          <option value="check">check</option>
          <option value="xmark">xmark</option>
          <option value="mail">mail</option>
          <option value="heart">heart</option>
          <option value="star">star</option>
          <option value="fire">fire</option>
          <option value="lightning">lightning</option>
          <option value="shield">shield</option>
          <option value="bug">bug</option>
          <option value="rocket">rocket</option>
          <option value="coffee">coffee</option>
          <option value="clock">clock</option>
          <option value="server">server</option>
          <option value="wifi">wifi</option>
          <option value="battery">battery</option>
          <option value="user">user</option>
          <option value="gear">gear</option>
          <option value="home">home</option>
          <option value="chart">chart</option>
          <option value="eye">eye</option>
          <option value="sun">sun</option>
          <option value="cloud">cloud</option>
          <option value="rain">rain</option>
          <option value="snow">snow</option>
          <option value="storm">storm</option>
          <option value="thermometer">thermometer</option>
          <option value="skull">skull</option>
          <option value="cat">cat</option>
        </select>
      </div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-notification" onclick="sendNotification()">Send Notification</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('notification')">&#9654; curl command</button>
    <div class="curl-box" id="curl-notification"></div>
  </div>

  <!-- DASHBOARD -->
  <div class="panel" id="panel-dashboard">
    <div style="display:flex;gap:6px;align-items:center;margin-bottom:8px">
      <strong style="font-size:12px">Widgets</strong>
      <button class="btn btn-secondary" onclick="addWidget()" style="padding:4px 8px;font-size:11px">+ Add</button>
    </div>
    <div class="widget-list" id="widget-list">
      <div class="widget-item">
        <select onchange="updateDashCurl()"><option value="metric">Metric</option><option value="text">Text</option></select>
        <input type="text" placeholder="Label" value="CPU" onchange="updateDashCurl()">
        <input type="text" placeholder="Value" value="42%" onchange="updateDashCurl()">
        <button class="remove-btn" onclick="this.parentElement.remove();updateDashCurl()">x</button>
      </div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-dashboard" onclick="sendDashboard()">Update Dashboard</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('dashboard')">&#9654; curl command</button>
    <div class="curl-box" id="curl-dashboard"></div>
  </div>

  <!-- EMOJI -->
  <div class="panel" id="panel-emoji">
    <div class="form-group"><label>Emoji Name</label><input type="text" id="emoji-name" value="cat" placeholder="cat"></div>
    <div class="icon-grid" id="icon-grid"></div>
    <div class="form-row">
      <div class="form-group"><label>Size</label>
        <select id="emoji-size"><option value="fullscreen">Fullscreen</option><option value="normal">Normal</option></select>
      </div>
      <div class="form-group"><label>Caption</label><input type="text" id="emoji-caption" placeholder="MEOW"></div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-emoji" onclick="sendEmoji()">Display Emoji</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('emoji')">&#9654; curl command</button>
    <div class="curl-box" id="curl-emoji"></div>
  </div>

  <!-- QR CODE -->
  <div class="panel" id="panel-qrcode">
    <div class="form-group"><label>Data / URL</label><input type="text" id="qr-data" value="https://github.com" placeholder="https://example.com"></div>
    <div class="form-group"><label>Caption</label><input type="text" id="qr-caption" placeholder="Scan me"></div>
    <div class="actions">
      <button class="btn" id="btn-qrcode" onclick="sendQR()">Generate QR Code</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('qrcode')">&#9654; curl command</button>
    <div class="curl-box" id="curl-qrcode"></div>
  </div>

  <!-- WEATHER -->
  <div class="panel" id="panel-weather">
    <div style="display:flex;gap:6px;margin-bottom:10px">
      <button class="btn btn-secondary" id="wx-mode-live" onclick="setWxMode('live')" style="padding:5px 12px;font-size:11px;background:var(--accent);color:#fff">Live (API)</button>
      <button class="btn btn-secondary" id="wx-mode-manual" onclick="setWxMode('manual')" style="padding:5px 12px;font-size:11px">Manual</button>
    </div>
    <!-- Live weather -->
    <div id="wx-live">
      <div class="form-row">
        <div class="form-group"><label>Location Name</label><input type="text" id="wx-api-location" value="London" placeholder="London"></div>
        <div class="form-group"><label>Latitude</label><input type="text" id="wx-api-lat" value="51.51" placeholder="51.51"></div>
      </div>
      <div class="form-row">
        <div class="form-group"><label>Longitude</label><input type="text" id="wx-api-lon" value="-0.13" placeholder="-0.13"></div>
        <div class="form-group">
          <label>Common Locations</label>
          <select id="wx-presets" onchange="applyWxPreset()">
            <option value="">-- Select --</option>
            <option value="51.51,-0.13,London">London</option>
            <option value="40.71,-74.01,New York">New York</option>
            <option value="48.86,2.35,Paris">Paris</option>
            <option value="35.68,139.69,Tokyo">Tokyo</option>
            <option value="-33.87,151.21,Sydney">Sydney</option>
            <option value="37.77,-122.42,San Francisco">San Francisco</option>
            <option value="52.52,13.41,Berlin">Berlin</option>
            <option value="55.75,37.62,Moscow">Moscow</option>
            <option value="19.43,-99.13,Mexico City">Mexico City</option>
            <option value="28.61,77.21,New Delhi">New Delhi</option>
            <option value="1.35,103.82,Singapore">Singapore</option>
            <option value="25.20,55.27,Dubai">Dubai</option>
          </select>
        </div>
      </div>
      <div class="actions">
        <button class="btn" id="btn-weather-fetch" onclick="fetchWeather()">Fetch & Display Weather</button>
      </div>
      <div style="font-size:11px;color:var(--text2);margin-top:6px">Uses Open-Meteo API (free, no key required). ESP32 fetches directly.</div>
    </div>
    <!-- Manual weather -->
    <div id="wx-manual" style="display:none">
      <div class="form-row">
        <div class="form-group"><label>Temperature</label><input type="text" id="wx-temp" value="22" placeholder="22"></div>
        <div class="form-group"><label>Condition</label>
          <select id="wx-condition">
            <option value="sunny">Sunny</option><option value="cloudy">Cloudy</option>
            <option value="rainy">Rainy</option><option value="snowy">Snowy</option>
            <option value="stormy">Stormy</option>
          </select>
        </div>
      </div>
      <div class="form-row">
        <div class="form-group"><label>Humidity %</label><input type="text" id="wx-humidity" value="45" placeholder="45"></div>
        <div class="form-group"><label>Location</label><input type="text" id="wx-location" value="London" placeholder="London"></div>
      </div>
      <div class="actions">
        <button class="btn" id="btn-weather" onclick="sendWeather()">Display Weather</button>
      </div>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('weather')">&#9654; curl command</button>
    <div class="curl-box" id="curl-weather"></div>
  </div>

  <!-- COUNTDOWN -->
  <div class="panel" id="panel-countdown">
    <div class="form-group"><label>Title</label><input type="text" id="cd-title" value="Deploy" placeholder="Event name"></div>
    <div class="form-group"><label>Target Date/Time</label><input type="datetime-local" id="cd-target"></div>
    <div class="actions">
      <button class="btn" id="btn-countdown" onclick="sendCountdown()">Start Countdown</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('countdown')">&#9654; curl command</button>
    <div class="curl-box" id="curl-countdown"></div>
  </div>

  <!-- CLOCK -->
  <div class="panel" id="panel-clock">
    <div class="form-group"><label>Time Format</label>
      <select id="clk-format"><option value="24">24-hour</option><option value="12">12-hour</option></select>
    </div>
    <div id="tz-list">
      <div class="form-group"><label>Primary Timezone</label>
        <div class="form-row">
          <input type="text" class="tz-label" value="London" placeholder="City name">
          <select class="tz-offset">
            <option value="-720">UTC-12</option><option value="-660">UTC-11</option>
            <option value="-600">UTC-10 (Hawaii)</option><option value="-540">UTC-9 (Alaska)</option>
            <option value="-480">UTC-8 (PST)</option><option value="-420">UTC-7 (MST)</option>
            <option value="-360">UTC-6 (CST)</option><option value="-300">UTC-5 (EST)</option>
            <option value="-240">UTC-4 (AST)</option><option value="-180">UTC-3 (BRT)</option>
            <option value="-120">UTC-2</option><option value="-60">UTC-1</option>
            <option value="0" selected>UTC+0 (GMT)</option>
            <option value="60">UTC+1 (CET)</option><option value="120">UTC+2 (EET)</option>
            <option value="180">UTC+3 (MSK)</option><option value="210">UTC+3:30 (Tehran)</option>
            <option value="240">UTC+4 (Dubai)</option><option value="270">UTC+4:30 (Kabul)</option>
            <option value="300">UTC+5 (PKT)</option><option value="330">UTC+5:30 (IST)</option>
            <option value="345">UTC+5:45 (Nepal)</option><option value="360">UTC+6 (BST)</option>
            <option value="420">UTC+7 (ICT)</option><option value="480">UTC+8 (CST/SGT)</option>
            <option value="540">UTC+9 (JST/KST)</option><option value="570">UTC+9:30 (ACST)</option>
            <option value="600">UTC+10 (AEST)</option><option value="660">UTC+11</option>
            <option value="720">UTC+12 (NZST)</option><option value="780">UTC+13</option>
          </select>
        </div>
      </div>
    </div>
    <div style="display:flex;gap:6px;margin-bottom:10px">
      <button class="btn btn-secondary" onclick="addTimezone()" style="padding:5px 10px;font-size:11px">+ Add Timezone</button>
      <span style="font-size:11px;color:#888;align-self:center">(up to 4)</span>
    </div>
    <div class="actions">
      <button class="btn" id="btn-clock" onclick="sendClock()">Start Clock</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('clock')">&#9654; curl command</button>
    <div class="curl-box" id="curl-clock"></div>
  </div>

  <!-- BIG CLOCK -->
  <div class="panel" id="panel-bigclock">
    <div class="form-group"><label>Time Format</label>
      <select id="bc-format"><option value="24">24-hour</option><option value="12">12-hour</option></select>
    </div>
    <div class="form-row">
      <div class="form-group"><label>Timezone</label>
        <select id="bc-offset">
          <option value="-720">UTC-12</option><option value="-660">UTC-11</option>
          <option value="-600">UTC-10 (Hawaii)</option><option value="-540">UTC-9 (Alaska)</option>
          <option value="-480">UTC-8 (PST)</option><option value="-420">UTC-7 (MST)</option>
          <option value="-360">UTC-6 (CST)</option><option value="-300">UTC-5 (EST)</option>
          <option value="-240">UTC-4 (AST)</option><option value="-180">UTC-3 (BRT)</option>
          <option value="-120">UTC-2</option><option value="-60">UTC-1</option>
          <option value="0">UTC+0 (GMT)</option>
          <option value="60" selected>UTC+1 (CET)</option><option value="120">UTC+2 (EET)</option>
          <option value="180">UTC+3 (MSK)</option><option value="210">UTC+3:30 (Tehran)</option>
          <option value="240">UTC+4 (Dubai)</option><option value="270">UTC+4:30 (Kabul)</option>
          <option value="300">UTC+5 (PKT)</option><option value="330">UTC+5:30 (IST)</option>
          <option value="345">UTC+5:45 (Nepal)</option><option value="360">UTC+6 (BST)</option>
          <option value="420">UTC+7 (ICT)</option><option value="480">UTC+8 (CST/SGT)</option>
          <option value="540">UTC+9 (JST/KST)</option><option value="570">UTC+9:30 (ACST)</option>
          <option value="600">UTC+10 (AEST)</option><option value="660">UTC+11</option>
          <option value="720">UTC+12 (NZST)</option><option value="780">UTC+13</option>
        </select>
      </div>
      <div class="form-group"><label>Label</label>
        <input type="text" id="bc-label" value="London" placeholder="City name">
      </div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-bigclock" onclick="sendBigClock()">Start Big Clock</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('bigclock')">&#9654; curl command</button>
    <div class="curl-box" id="curl-bigclock"></div>
  </div>

  <!-- RSS -->
  <div class="panel" id="panel-rss">
    <div style="display:flex;gap:6px;align-items:center;margin-bottom:8px">
      <strong style="font-size:12px">RSS Feeds</strong>
      <button class="btn btn-secondary" onclick="addRssFeed()" style="padding:4px 8px;font-size:11px">+ Add Feed</button>
      <span style="font-size:11px;color:#888">(up to 3)</span>
    </div>

    <div class="form-group"><label>Quick Add</label>
      <select id="rss-presets" onchange="addRssPreset(this.value);this.value=''">
        <option value="">-- Select a feed --</option>
        <option value="BBC News|https://feeds.bbci.co.uk/news/rss.xml">BBC News</option>
        <option value="BBC Tech|https://feeds.bbci.co.uk/news/technology/rss.xml">BBC Tech</option>
        <option value="Reuters|https://feeds.reuters.com/reuters/topNews">Reuters Top</option>
        <option value="Hacker News|https://hnrss.org/frontpage">Hacker News</option>
        <option value="Ars Technica|https://feeds.arstechnica.com/arstechnica/index">Ars Technica</option>
        <option value="The Verge|https://www.theverge.com/rss/index.xml">The Verge</option>
        <option value="NASA|https://www.nasa.gov/rss/dyn/breaking_news.rss">NASA Breaking</option>
        <option value="ESPN|https://www.espn.com/espn/rss/news">ESPN</option>
        <option value="NY Times|https://rss.nytimes.com/services/xml/rss/nyt/HomePage.xml">NY Times</option>
        <option value="TechCrunch|https://techcrunch.com/feed/">TechCrunch</option>
        <option value="Reddit Tech|https://www.reddit.com/r/technology/.rss">Reddit Tech</option>
        <option value="Wired|https://www.wired.com/feed/rss">Wired</option>
      </select>
    </div>
    <div class="rss-feed-list" id="rss-feed-list">
      <div class="rss-feed-item">
        <input type="text" placeholder="Name" value="BBC" class="rss-name">
        <input type="text" placeholder="Feed URL" value="https://feeds.bbci.co.uk/news/rss.xml" class="rss-url">
        <button class="remove-btn" onclick="this.parentElement.remove()" style="background:var(--red);color:#fff;border:none;padding:3px 7px;border-radius:4px;cursor:pointer;font-size:14px">x</button>
      </div>
    </div>
    <div class="form-row">
      <div class="form-group"><label>Items per page</label><input type="number" id="rss-items" value="4" min="1" max="8"></div>
      <div class="form-group"><label>Refresh interval (sec)</label><input type="number" id="rss-interval" value="300" min="60"></div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-rss" onclick="sendRss()">Start RSS Feed</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('rss')">&#9654; curl command</button>
    <div class="curl-box" id="curl-rss"></div>
  </div>

  <!-- IMAGE -->
  <div class="panel" id="panel-image">
    <div class="form-group"><label>Upload Image (any format: JPG, PNG, BMP, GIF, WebP)</label>
      <input type="file" id="img-file" accept="image/*" style="padding:6px">
    </div>
    <canvas id="img-preview" style="display:none;max-width:100%;border:1px solid var(--border);border-radius:6px;margin:8px 0"></canvas>
    <div id="img-info" style="font-size:11px;color:#888;margin-bottom:8px"></div>
    <div class="actions">
      <button class="btn" id="btn-image" onclick="sendImage()">Upload & Display</button>
    </div>
  </div>

  <!-- SYSTEM -->
  <div class="panel" id="panel-system">
    <div class="sys-section">
      <h3>Device Info</h3>
      <div class="sys-grid">
        <div class="sys-item"><div class="label">IP Address</div><div class="value" id="sys-ip">--</div></div>
        <div class="sys-item"><div class="label">WiFi</div><div class="value" id="sys-ssid">--</div></div>
        <div class="sys-item"><div class="label">RSSI</div><div class="value" id="sys-rssi">--</div></div>
        <div class="sys-item"><div class="label">Free Heap</div><div class="value" id="sys-heap">--</div></div>
        <div class="sys-item"><div class="label">Uptime</div><div class="value" id="sys-uptime">--</div></div>
        <div class="sys-item"><div class="label">Last Update</div><div class="value" id="sys-last">--</div></div>
      </div>
    </div>

    <div class="sys-section">
      <h3>Display Settings</h3>
      <div class="form-row">
        <div class="form-group"><label>Rotation</label>
          <select id="sys-rotation" onchange="setRotation()">
            <option value="0">0 (Normal)</option><option value="1">90</option><option value="2">180</option><option value="3">270</option>
          </select>
        </div>
        <div class="form-group"><label>Quick Actions</label>
          <div style="display:flex;gap:4px;margin-top:2px">
            <button class="btn btn-secondary" onclick="clearDisplay()" style="padding:6px 10px;font-size:11px">Clear</button>
            <button class="btn btn-secondary" onclick="invertDisplay()" style="padding:6px 10px;font-size:11px">Invert</button>
          </div>
        </div>
      </div>
    </div>

    <div class="sys-section">
      <h3>API Authentication</h3>
      <div class="auth-section">
        <div class="auth-status" id="auth-status"></div>
        <div class="form-group" id="auth-current-group" style="display:none">
          <label>Current API Key</label>
          <input type="password" id="auth-current" placeholder="Enter current key">
        </div>
        <div class="form-group">
          <label>New API Key (leave blank to disable auth)</label>
          <input type="text" id="auth-new" placeholder="Enter API key or leave empty">
        </div>
        <button class="btn" id="btn-auth" onclick="setApiKey()">Update API Key</button>
        <div style="font-size:11px;color:var(--text2);margin-top:6px">
          When set, all API requests require <code style="color:var(--green)">X-API-Key</code> header. The web UI stores the key locally.
        </div>
      </div>
    </div>

    <div class="sys-section">
      <h3>Danger Zone</h3>
      <div class="danger-zone">
        <p>Reset WiFi credentials and reboot into setup mode.</p>
        <button class="btn btn-danger" id="btn-wifi-reset" onclick="resetWiFi()">Reset WiFi Settings</button>
      </div>
    </div>
  </div>

  <div class="history" id="history-section">
    <h3>Recent Updates</h3>
    <div id="history-list"></div>
  </div>
</div>

<script>
const BASE = '';
const icons = ['warning','check','x-mark','heart','cat','skull','fire','bell','mail','clock',
  'server','wifi','battery','star','sun','cloud','rain','snow','storm','thermometer',
  'coffee','rocket','bug','shield','eye','lightning','gear','home','chart','user'];
const historyItems = [];
let storedApiKey = localStorage.getItem('epaper_api_key') || '';
let authEnabled = false;

function toast(msg, type) {
  const c = document.getElementById('toast-container');
  const t = document.createElement('div');
  t.className = 'toast ' + type;
  t.textContent = msg;
  c.appendChild(t);
  setTimeout(() => { t.style.opacity = '0'; t.style.transition = 'opacity .3s'; setTimeout(() => t.remove(), 300); }, 3000);
}

function btnLoading(id, loading) {
  const btn = document.getElementById('btn-' + id);
  if (!btn) return;
  if (loading) {
    btn._origText = btn.textContent;
    btn.disabled = true;
    btn.innerHTML = '<span class="spinner"></span>Sending...';
  } else {
    btn.disabled = false;
    btn.textContent = btn._origText || 'Send';
  }
}

function toggleCurl(id) {
  const box = document.getElementById('curl-' + id);
  if (box) box.classList.toggle('show');
}

// Init
document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.tab').forEach(t => {
    t.addEventListener('click', () => {
      document.querySelectorAll('.tab').forEach(x => x.classList.remove('active'));
      document.querySelectorAll('.panel').forEach(x => x.classList.remove('active'));
      t.classList.add('active');
      document.getElementById('panel-' + t.dataset.tab).classList.add('active');
    });
  });

  const grid = document.getElementById('icon-grid');
  icons.forEach(name => {
    const d = document.createElement('div');
    d.className = 'icon-opt';
    d.textContent = name;
    d.onclick = () => {
      document.querySelectorAll('.icon-opt').forEach(x => x.classList.remove('selected'));
      d.classList.add('selected');
      document.getElementById('emoji-name').value = name;
    };
    grid.appendChild(d);
  });

  const dt = new Date(Date.now() + 3600000);
  document.getElementById('cd-target').value = dt.toISOString().slice(0,16);

  fetchStatus();
  setInterval(fetchStatus, 10000);
});

async function fetchStatus() {
  try {
    const r = await fetch(BASE + '/api/status');
    const d = await r.json();
    document.getElementById('st-ip').textContent = d.ip || '--';
    document.getElementById('st-rssi').textContent = (d.rssi || '--') + ' dBm';
    const up = d.uptime || 0;
    const h = Math.floor(up/3600), m = Math.floor((up%3600)/60);
    document.getElementById('st-uptime').textContent = h + 'h ' + m + 'm';
    document.getElementById('st-heap').textContent = Math.round((d.heap||0)/1024) + ' KB';
    if (d.ssid) document.getElementById('st-ssid').textContent = d.ssid;
    // System tab
    document.getElementById('sys-ip').textContent = d.ip || '--';
    document.getElementById('sys-ssid').textContent = d.ssid || '--';
    document.getElementById('sys-rssi').textContent = (d.rssi || '--') + ' dBm';
    document.getElementById('sys-heap').textContent = Math.round((d.heap||0)/1024) + ' KB';
    document.getElementById('sys-uptime').textContent = h + 'h ' + m + 'm';
    document.getElementById('sys-last').textContent = d.last_update || '--';
    // Auth status
    authEnabled = d.auth_enabled || false;
    updateAuthUI();
  } catch(e) {}
}

function updateAuthUI() {
  const el = document.getElementById('auth-status');
  const curGroup = document.getElementById('auth-current-group');
  if (authEnabled) {
    el.className = 'auth-status locked';
    el.textContent = 'API authentication is ENABLED. Requests require X-API-Key header.';
    curGroup.style.display = 'block';
  } else {
    el.className = 'auth-status open';
    el.textContent = 'API authentication is DISABLED. All requests are allowed.';
    curGroup.style.display = 'none';
  }
}

function showCurl(id, endpoint, data) {
  const el = document.getElementById('curl-' + id);
  if (!el) return;
  const host = location.hostname || 'epaper.local';
  const port = location.port ? ':' + location.port : '';
  const json = JSON.stringify(data);
  let keyHeader = storedApiKey ? " -H 'X-API-Key: " + storedApiKey + "'" : '';
  const cmd = "curl -X POST http://" + host + port + endpoint + " -H 'Content-Type: application/json'" + keyHeader + " -d '" + json + "'";
  el.innerHTML = cmd + '<button class="copy-btn" onclick="navigator.clipboard.writeText(this.parentElement.textContent.replace(\'Copy\',\'\').trim())">Copy</button>';
}

function addHistory(type) {
  const now = new Date().toLocaleTimeString();
  historyItems.unshift({type, time: now});
  if (historyItems.length > 10) historyItems.pop();
  const list = document.getElementById('history-list');
  list.innerHTML = historyItems.map(h =>
    '<div class="history-item"><span class="type">' + h.type + '</span><span>' + h.time + '</span></div>'
  ).join('');
}

async function apiPost(endpoint, data) {
  const headers = {'Content-Type':'application/json'};
  if (storedApiKey) headers['X-API-Key'] = storedApiKey;
  const r = await fetch(BASE + endpoint, {
    method:'POST', headers, body:JSON.stringify(data)
  });
  const d = await r.json();
  if (d.error === 'Invalid or missing API key') {
    toast('Authentication failed. Check API key in System tab.', 'error');
  }
  return d;
}

async function sendText() {
  const data = {
    text: document.getElementById('txt-text').value,
    size: parseInt(document.getElementById('txt-size').value),
    x: parseInt(document.getElementById('txt-x').value),
    y: parseInt(document.getElementById('txt-y').value),
    align: document.getElementById('txt-align').value
  };
  showCurl('text', '/api/text', data);
  btnLoading('text', true);
  try {
    const r = await apiPost('/api/text', data);
    toast(r.ok ? 'Text displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('text');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('text', false);
}

async function sendNotification() {
  const data = {
    title: document.getElementById('notif-title').value,
    body: document.getElementById('notif-body').value,
    icon: document.getElementById('notif-icon').value,
    style: document.getElementById('notif-style').value
  };
  showCurl('notification', '/api/notification', data);
  btnLoading('notification', true);
  try {
    const r = await apiPost('/api/notification', data);
    toast(r.ok ? 'Notification displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('notification');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('notification', false);
}

function addWidget() {
  const list = document.getElementById('widget-list');
  if (list.children.length >= 6) { toast('Maximum 6 widgets', 'error'); return; }
  const item = document.createElement('div');
  item.className = 'widget-item';
  item.innerHTML = '<select onchange="updateDashCurl()"><option value="metric">Metric</option><option value="text">Text</option></select>' +
    '<input type="text" placeholder="Label" value="" onchange="updateDashCurl()">' +
    '<input type="text" placeholder="Value" value="" onchange="updateDashCurl()">' +
    '<button class="remove-btn" onclick="this.parentElement.remove();updateDashCurl()">x</button>';
  list.appendChild(item);
}

function getDashData() {
  const widgets = [];
  document.querySelectorAll('.widget-item').forEach(item => {
    const inputs = item.querySelectorAll('input');
    const select = item.querySelector('select');
    widgets.push({type: select.value, label: inputs[0].value, value: inputs[1].value});
  });
  return {widgets};
}

function updateDashCurl() { showCurl('dashboard', '/api/dashboard', getDashData()); }

async function sendDashboard() {
  const data = getDashData();
  showCurl('dashboard', '/api/dashboard', data);
  btnLoading('dashboard', true);
  try {
    const r = await apiPost('/api/dashboard', data);
    toast(r.ok ? 'Dashboard displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('dashboard');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('dashboard', false);
}

async function sendEmoji() {
  const data = {
    emoji: document.getElementById('emoji-name').value,
    size: document.getElementById('emoji-size').value,
    caption: document.getElementById('emoji-caption').value
  };
  showCurl('emoji', '/api/emoji', data);
  btnLoading('emoji', true);
  try {
    const r = await apiPost('/api/emoji', data);
    toast(r.ok ? 'Emoji displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('emoji');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('emoji', false);
}

async function sendQR() {
  const data = {
    data: document.getElementById('qr-data').value,
    caption: document.getElementById('qr-caption').value
  };
  showCurl('qrcode', '/api/qrcode', data);
  btnLoading('qrcode', true);
  try {
    const r = await apiPost('/api/qrcode', data);
    toast(r.ok ? 'QR code displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('qrcode');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('qrcode', false);
}

function setWxMode(mode) {
  document.getElementById('wx-live').style.display = mode === 'live' ? 'block' : 'none';
  document.getElementById('wx-manual').style.display = mode === 'manual' ? 'block' : 'none';
  document.getElementById('wx-mode-live').style.background = mode === 'live' ? 'var(--accent)' : 'var(--border)';
  document.getElementById('wx-mode-live').style.color = mode === 'live' ? '#fff' : '#ccc';
  document.getElementById('wx-mode-manual').style.background = mode === 'manual' ? 'var(--accent)' : 'var(--border)';
  document.getElementById('wx-mode-manual').style.color = mode === 'manual' ? '#fff' : '#ccc';
}

function applyWxPreset() {
  const val = document.getElementById('wx-presets').value;
  if (!val) return;
  const parts = val.split(',');
  document.getElementById('wx-api-lat').value = parts[0];
  document.getElementById('wx-api-lon').value = parts[1];
  document.getElementById('wx-api-location').value = parts[2];
}

async function fetchWeather() {
  const data = {
    lat: parseFloat(document.getElementById('wx-api-lat').value),
    lon: parseFloat(document.getElementById('wx-api-lon').value),
    location: document.getElementById('wx-api-location').value
  };
  showCurl('weather', '/api/wx', data);
  btnLoading('weather-fetch', true);
  try {
    const r = await apiPost('/api/wx', data);
    if (r.ok) {
      toast('Weather: ' + r.temp + ' ' + r.condition + ' ' + r.humidity + '% humidity', 'success');
      addHistory('weather');
    } else {
      toast(r.error || 'Weather fetch failed', 'error');
    }
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('weather-fetch', false);
}

async function sendWeather() {
  const data = {
    temp: document.getElementById('wx-temp').value,
    condition: document.getElementById('wx-condition').value,
    humidity: document.getElementById('wx-humidity').value,
    location: document.getElementById('wx-location').value
  };
  showCurl('weather', '/api/weather', data);
  btnLoading('weather', true);
  try {
    const r = await apiPost('/api/weather', data);
    toast(r.ok ? 'Weather displayed' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('weather');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('weather', false);
}

async function sendCountdown() {
  const target = document.getElementById('cd-target').value;
  const data = {
    title: document.getElementById('cd-title').value,
    target: new Date(target).toISOString()
  };
  showCurl('countdown', '/api/countdown', data);
  btnLoading('countdown', true);
  try {
    const r = await apiPost('/api/countdown', data);
    toast(r.ok ? 'Countdown started' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('countdown');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('countdown', false);
}

function addTimezone() {
  const list = document.getElementById('tz-list');
  if (list.querySelectorAll('.form-group').length >= 4) { toast('Maximum 4 timezones', 'error'); return; }
  const n = list.querySelectorAll('.form-group').length + 1;
  const div = document.createElement('div');
  div.className = 'form-group';
  div.innerHTML = '<label>Timezone ' + n + ' <span style="cursor:pointer;color:#f87171;float:right" onclick="this.parentElement.parentElement.remove()">&times; remove</span></label>' +
    '<div class="form-row"><input type="text" class="tz-label" placeholder="City name">' +
    '<select class="tz-offset">' +
    '<option value="-720">UTC-12</option><option value="-660">UTC-11</option>' +
    '<option value="-600">UTC-10</option><option value="-540">UTC-9</option>' +
    '<option value="-480">UTC-8 (PST)</option><option value="-420">UTC-7 (MST)</option>' +
    '<option value="-360">UTC-6 (CST)</option><option value="-300">UTC-5 (EST)</option>' +
    '<option value="-240">UTC-4</option><option value="-180">UTC-3</option>' +
    '<option value="-120">UTC-2</option><option value="-60">UTC-1</option>' +
    '<option value="0" selected>UTC+0 (GMT)</option>' +
    '<option value="60">UTC+1 (CET)</option><option value="120">UTC+2 (EET)</option>' +
    '<option value="180">UTC+3 (MSK)</option><option value="210">UTC+3:30</option>' +
    '<option value="240">UTC+4</option><option value="270">UTC+4:30</option>' +
    '<option value="300">UTC+5 (PKT)</option><option value="330">UTC+5:30 (IST)</option>' +
    '<option value="345">UTC+5:45</option><option value="360">UTC+6</option>' +
    '<option value="420">UTC+7</option><option value="480">UTC+8 (SGT)</option>' +
    '<option value="540">UTC+9 (JST)</option><option value="570">UTC+9:30</option>' +
    '<option value="600">UTC+10 (AEST)</option><option value="660">UTC+11</option>' +
    '<option value="720">UTC+12 (NZST)</option><option value="780">UTC+13</option>' +
    '</select></div>';
  list.appendChild(div);
}

function getClockData() {
  const timezones = [];
  document.querySelectorAll('#tz-list .form-group').forEach(g => {
    const label = g.querySelector('.tz-label').value || 'UTC';
    const offset = parseInt(g.querySelector('.tz-offset').value);
    timezones.push({label, offset});
  });
  return { hour24: document.getElementById('clk-format').value === '24', timezones };
}

async function sendClock() {
  const data = getClockData();
  showCurl('clock', '/api/clock', data);
  btnLoading('clock', true);
  try {
    const r = await apiPost('/api/clock', data);
    toast(r.ok ? 'Clock started' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('clock');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('clock', false);
}

// Big Clock
async function sendBigClock() {
  const data = {
    hour24: document.getElementById('bc-format').value === '24',
    offset: parseInt(document.getElementById('bc-offset').value),
    label: document.getElementById('bc-label').value || ''
  };
  showCurl('bigclock', '/api/bigclock', data);
  btnLoading('bigclock', true);
  try {
    const r = await apiPost('/api/bigclock', data);
    toast(r.ok ? 'Big clock started' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('bigclock');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('bigclock', false);
}

// RSS functions
function addRssPreset(val) {
  if (!val) return;
  const list = document.getElementById('rss-feed-list');
  if (list.children.length >= 3) { toast('Maximum 3 feeds', 'error'); return; }
  const parts = val.split('|');
  const item = document.createElement('div');
  item.className = 'rss-feed-item';
  item.innerHTML = '<input type="text" placeholder="Name" value="' + parts[0] + '" class="rss-name">' +
    '<input type="text" placeholder="Feed URL" value="' + parts[1] + '" class="rss-url">' +
    '<button onclick="this.parentElement.remove()" style="background:var(--red);color:#fff;border:none;padding:3px 7px;border-radius:4px;cursor:pointer;font-size:14px">x</button>';
  list.appendChild(item);
}

function addRssFeed() {
  const list = document.getElementById('rss-feed-list');
  if (list.children.length >= 3) { toast('Maximum 3 feeds', 'error'); return; }
  const item = document.createElement('div');
  item.className = 'rss-feed-item';
  item.innerHTML = '<input type="text" placeholder="Name" class="rss-name">' +
    '<input type="text" placeholder="Feed URL" class="rss-url">' +
    '<button onclick="this.parentElement.remove()" style="background:var(--red);color:#fff;border:none;padding:3px 7px;border-radius:4px;cursor:pointer;font-size:14px">x</button>';
  list.appendChild(item);
}

function getRssData() {
  const urls = [], names = [];
  document.querySelectorAll('.rss-feed-item').forEach(item => {
    const url = item.querySelector('.rss-url').value.trim();
    const name = item.querySelector('.rss-name').value.trim() || 'Feed';
    if (url) { urls.push(url); names.push(name); }
  });
  return {
    urls, names,
    items: parseInt(document.getElementById('rss-items').value) || 4,
    interval: (parseInt(document.getElementById('rss-interval').value) || 300) * 1000
  };
}

async function sendRss() {
  const data = getRssData();
  if (data.urls.length === 0) { toast('Add at least one feed URL', 'error'); return; }
  showCurl('rss', '/api/rss', data);
  btnLoading('rss', true);
  try {
    const r = await apiPost('/api/rss', data);
    toast(r.ok ? 'RSS feed started. Fetching...' : (r.error || 'Failed'), r.ok ? 'success' : 'error');
    if (r.ok) addHistory('rss');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('rss', false);
}

// Convert image to 400x300 raw 1-bit pixels with Floyd-Steinberg dithering
// Returns 15000 bytes: 1 bit per pixel, MSB first, top-down, no headers
function imageToRaw(file) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => {
      const W = 400, H = 300;
      const canvas = document.getElementById('img-preview');
      canvas.width = W; canvas.height = H;
      canvas.style.display = 'block';
      const ctx = canvas.getContext('2d');
      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, W, H);
      // Crop-to-fill: scale up to cover entire display, crop overflow
      const scale = Math.max(W / img.width, H / img.height);
      const dw = Math.round(img.width * scale);
      const dh = Math.round(img.height * scale);
      ctx.drawImage(img, Math.round((W-dw)/2), Math.round((H-dh)/2), dw, dh);
      const idata = ctx.getImageData(0, 0, W, H);

      // Grayscale buffer
      const gray = new Float32Array(W * H);
      for (let i = 0; i < W * H; i++) {
        const si = i * 4;
        gray[i] = idata.data[si] * 0.299 + idata.data[si+1] * 0.587 + idata.data[si+2] * 0.114;
      }

      // Floyd-Steinberg dithering
      const raw = new Uint8Array(W * H / 8); // 15000 bytes
      for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
          const i = y * W + x;
          const old = gray[i];
          const nw = old < 128 ? 0 : 255;
          const err = old - nw;
          if (x+1 < W) gray[i+1] += err * 7/16;
          if (y+1 < H) {
            if (x > 0) gray[(y+1)*W+x-1] += err * 3/16;
            gray[(y+1)*W+x] += err * 5/16;
            if (x+1 < W) gray[(y+1)*W+x+1] += err * 1/16;
          }
          // Pack: bit=1 means black pixel, MSB first
          if (nw === 0) {
            raw[(y * W + x) >> 3] |= (0x80 >> (x & 7));
          }
          // Update preview
          idata.data[i*4] = idata.data[i*4+1] = idata.data[i*4+2] = nw;
        }
      }
      ctx.putImageData(idata, 0, 0);
      document.getElementById('img-info').textContent =
        'Original: ' + img.width + 'x' + img.height + ' -> 400x300 dithered (15KB)';
      resolve(raw);
    };
    img.onerror = () => reject(new Error('Failed to load image'));
    img.src = URL.createObjectURL(file);
  });
}

// Preview on file select
document.getElementById('img-file').addEventListener('change', async function() {
  const file = this.files[0];
  if (!file) return;
  try { await imageToRaw(file); } catch(e) { /* preview only */ }
});

async function sendImage() {
  const file = document.getElementById('img-file').files[0];
  if (!file) { toast('No file selected', 'error'); return; }
  btnLoading('image', true);
  try {
    const raw = await imageToRaw(file);
    const headers = {'Content-Type': 'application/octet-stream'};
    if (storedApiKey) headers['X-API-Key'] = storedApiKey;
    const r = await fetch(BASE + '/api/image', {method:'POST', headers, body: raw});
    const d = await r.json();
    toast(d.ok ? 'Image displayed' : (d.error || 'Failed'), d.ok ? 'success' : 'error');
    if (d.ok) addHistory('image');
  } catch(e) { toast('Error: ' + e.message, 'error'); }
  btnLoading('image', false);
}

async function clearDisplay() {
  try {
    const r = await apiPost('/api/clear', {});
    toast(r.ok ? 'Display cleared' : 'Failed', r.ok ? 'success' : 'error');
    if (r.ok) addHistory('clear');
  } catch(e) { toast('Connection error', 'error'); }
}

async function invertDisplay() {
  try {
    const r = await apiPost('/api/invert', {});
    toast(r.ok ? 'Display inverted' : 'Failed', r.ok ? 'success' : 'error');
    if (r.ok) addHistory('invert');
  } catch(e) { toast('Connection error', 'error'); }
}

async function setRotation() {
  const rot = parseInt(document.getElementById('sys-rotation').value);
  try {
    const r = await apiPost('/api/config', {rotation: rot});
    toast(r.ok ? 'Rotation set to ' + (rot * 90) + ' degrees' : 'Failed', r.ok ? 'success' : 'error');
  } catch(e) { toast('Connection error', 'error'); }
}

async function setApiKey() {
  const currentKey = document.getElementById('auth-current').value;
  const newKey = document.getElementById('auth-new').value;

  btnLoading('auth', true);
  try {
    const headers = {'Content-Type':'application/json'};
    const r = await fetch(BASE + '/api/auth', {
      method:'POST', headers,
      body: JSON.stringify({current_key: currentKey, new_key: newKey})
    });
    const d = await r.json();
    if (d.ok) {
      storedApiKey = newKey;
      localStorage.setItem('epaper_api_key', newKey);
      toast(newKey ? 'API key set. Stored in browser.' : 'API authentication disabled.', 'success');
      document.getElementById('auth-current').value = '';
      document.getElementById('auth-new').value = '';
      fetchStatus();
    } else {
      toast(d.error || 'Failed to update key', 'error');
    }
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('auth', false);
}

async function resetWiFi() {
  if (!confirm('Reset WiFi credentials? The device will reboot into setup mode.')) return;
  const btn = document.getElementById('btn-wifi-reset');
  btn.disabled = true;
  btn.innerHTML = '<span class="spinner"></span>Resetting...';
  try {
    await apiPost('/api/wifi/reset', {});
    toast('WiFi reset. Device is rebooting...', 'success');
  } catch(e) {
    toast('Device is rebooting into setup mode', 'success');
  }
}
</script>
</body>
</html>
)rawliteral";
