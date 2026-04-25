#pragma once

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>E-Paper Display Control</title>
<style>
:root{
  --bg:#111;--bg2:#1a1a1a;--bg3:#222;--border:#333;--border2:#444;
  --text:#e0e0e0;--text2:#888;--text3:#999;--accent:#2563eb;--accent2:#1d4ed8;
  --green:#4ade80;--red:#dc2626;--red2:#b91c1c;
}
*{margin:0;padding:0;box-sizing:border-box}
body{background:var(--bg);color:var(--text);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}

/* Header */
.header{background:var(--bg2);border-bottom:1px solid var(--border);padding:14px 24px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px}
.header-left{display:flex;align-items:center;gap:10px}
.header h1{font-size:18px;font-weight:600;color:#fff}
.pulse{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 6px var(--green);animation:pulse-anim 2s infinite}
@keyframes pulse-anim{0%,100%{opacity:1}50%{opacity:.4}}
.status-bar{display:flex;gap:14px;font-size:12px;color:var(--text2);flex-wrap:wrap}
.status-bar .val{color:var(--green)}

/* Container */
.container{max-width:900px;margin:0 auto;padding:20px}

/* Tabs */
.tabs{display:flex;gap:6px;margin-bottom:18px;flex-wrap:wrap;overflow-x:auto;-webkit-overflow-scrolling:touch;scrollbar-width:none}
.tabs::-webkit-scrollbar{display:none}
.tab{padding:8px 16px;background:var(--bg2);border:1px solid var(--border);border-radius:20px;cursor:pointer;font-size:13px;color:var(--text3);transition:all .2s;white-space:nowrap;user-select:none}
.tab:hover{background:var(--bg3);color:#ccc}
.tab.active{background:var(--accent);border-color:var(--accent);color:#fff}
.tab[data-tab="system"]{border-color:#555}
.tab[data-tab="system"].active{background:#6366f1;border-color:#6366f1}

/* Panel */
.panel{display:none;background:var(--bg2);border:1px solid var(--border);border-radius:12px;padding:24px}
.panel.active{display:block}

/* Forms */
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:12px;color:var(--text2);margin-bottom:5px;font-weight:500;text-transform:uppercase;letter-spacing:.5px}
.form-group input,.form-group textarea,.form-group select{width:100%;padding:10px 14px;background:var(--bg3);border:1px solid var(--border2);border-radius:8px;color:var(--text);font-size:14px;outline:none;transition:border .2s}
.form-group input:focus,.form-group textarea:focus,.form-group select:focus{border-color:var(--accent)}
.form-group textarea{resize:vertical;min-height:70px}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.form-row-3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}

/* Buttons */
.btn{padding:10px 20px;background:var(--accent);color:#fff;border:none;border-radius:8px;cursor:pointer;font-size:13px;font-weight:600;transition:all .15s;display:inline-flex;align-items:center;gap:6px}
.btn:hover{background:var(--accent2)}
.btn:active{transform:scale(0.98)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.btn-secondary{background:var(--border);color:#ccc}
.btn-secondary:hover{background:var(--border2)}
.btn-danger{background:var(--red)}
.btn-danger:hover{background:var(--red2)}
.btn .spinner{display:inline-block;width:14px;height:14px;border:2px solid #fff4;border-top-color:#fff;border-radius:50%;animation:spin .6s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
.actions{display:flex;gap:8px;margin-top:16px;flex-wrap:wrap}

/* Curl box */
.curl-toggle{margin-top:12px;background:none;border:none;color:var(--text2);cursor:pointer;font-size:12px;padding:4px 0;display:flex;align-items:center;gap:4px}
.curl-toggle:hover{color:var(--text)}
.curl-box{margin-top:6px;background:#0a0a0a;border:1px solid var(--border);border-radius:8px;padding:12px;font-family:'Courier New',monospace;font-size:11px;color:var(--green);word-break:break-all;position:relative;display:none}
.curl-box.show{display:block}
.curl-box .copy-btn{position:absolute;top:6px;right:6px;background:var(--border);color:#ccc;border:none;padding:3px 8px;border-radius:4px;cursor:pointer;font-size:10px}
.curl-box .copy-btn:hover{background:var(--border2)}

/* Toast */
.toast-container{position:fixed;top:16px;right:16px;z-index:1000;display:flex;flex-direction:column;gap:8px}
.toast{padding:12px 18px;border-radius:10px;font-size:13px;color:#fff;animation:toast-in .3s;max-width:320px;box-shadow:0 4px 12px #0008}
.toast.success{background:#166534;border:1px solid #22c55e}
.toast.error{background:#7f1d1d;border:1px solid #f87171}
@keyframes toast-in{from{opacity:0;transform:translateX(40px)}to{opacity:1;transform:none}}

/* Icon grid */
.icon-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(72px,1fr));gap:6px;margin-top:6px}
.icon-opt{padding:7px;background:var(--bg3);border:2px solid var(--border);border-radius:8px;cursor:pointer;text-align:center;font-size:11px;color:var(--text2);transition:all .15s}
.icon-opt:hover{border-color:#555;color:#ccc}
.icon-opt.selected{border-color:var(--accent);color:#fff;background:#1e3a5f}

/* Widget list */
.widget-list{margin-top:10px}
.widget-item{background:var(--bg3);border:1px solid var(--border);border-radius:8px;padding:10px;margin-bottom:6px;display:grid;grid-template-columns:1fr 1fr 1fr auto;gap:8px;align-items:center}
.widget-item input,.widget-item select{padding:6px 8px;background:var(--bg2);border:1px solid var(--border2);border-radius:4px;color:var(--text);font-size:13px}
.widget-item .remove-btn{background:var(--red);color:#fff;border:none;padding:4px 8px;border-radius:4px;cursor:pointer;font-size:16px}

/* System panel */
.sys-section{margin-bottom:20px}
.sys-section h3{font-size:14px;color:var(--text2);margin-bottom:10px;text-transform:uppercase;letter-spacing:.5px}
.sys-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.sys-item{background:var(--bg3);border:1px solid var(--border);border-radius:8px;padding:12px}
.sys-item .label{font-size:11px;color:var(--text2);text-transform:uppercase;letter-spacing:.5px;margin-bottom:4px}
.sys-item .value{font-size:15px;color:var(--green);font-weight:600}
.danger-zone{border:1px solid #7f1d1d;border-radius:8px;padding:16px;background:#1a0000}
.danger-zone p{font-size:13px;color:var(--text2);margin-bottom:12px}

/* History */
.history{margin-top:20px}
.history h3{font-size:13px;color:var(--text2);margin-bottom:8px;text-transform:uppercase;letter-spacing:.5px}
.history-item{padding:7px 10px;background:var(--bg2);border:1px solid var(--bg3);border-radius:6px;margin-bottom:3px;font-size:12px;color:var(--text3);display:flex;justify-content:space-between}
.history-item .type{color:var(--green);font-weight:600}

/* Mobile */
@media(max-width:480px){
  .header{padding:12px 16px}
  .header h1{font-size:16px}
  .status-bar{font-size:11px;gap:8px}
  .container{padding:12px}
  .panel{padding:16px}
  .form-row,.form-row-3,.sys-grid{grid-template-columns:1fr}
  .widget-item{grid-template-columns:1fr 1fr;gap:6px}
  .btn{padding:12px 16px;min-height:44px;font-size:14px}
  .tab{padding:10px 14px;min-height:44px;display:flex;align-items:center}
}
</style>
</head>
<body>
<div class="header">
  <div class="header-left">
    <div class="pulse"></div>
    <h1>E-Paper Display</h1>
    <span style="font-size:12px;color:#888" id="st-ssid"></span>
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
    <div class="tab" data-tab="image">Image</div>
    <div class="tab" data-tab="system">System</div>
  </div>

  <!-- TEXT -->
  <div class="panel active" id="panel-text">
    <div class="form-group"><label>Text</label><textarea id="txt-text" placeholder="Hello World">Hello World</textarea></div>
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
      <button class="btn btn-danger" onclick="clearDisplay()">Clear</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('text')">&#9654; curl command</button>
    <div class="curl-box" id="curl-text"></div>
  </div>

  <!-- NOTIFICATION -->
  <div class="panel" id="panel-notification">
    <div class="form-group"><label>Title</label><input type="text" id="notif-title" placeholder="Deploy Failed" value="Alert"></div>
    <div class="form-group"><label>Body</label><textarea id="notif-body" placeholder="Build #482 crashed on staging">Something happened</textarea></div>
    <div class="form-row">
      <div class="form-group"><label>Style</label>
        <select id="notif-style"><option value="card">Card</option><option value="banner">Banner</option><option value="fullscreen">Fullscreen</option></select>
      </div>
      <div class="form-group"><label>Icon</label><input type="text" id="notif-icon" value="warning" placeholder="warning"></div>
    </div>
    <div class="actions">
      <button class="btn" id="btn-notification" onclick="sendNotification()">Send Notification</button>
    </div>
    <button class="curl-toggle" onclick="toggleCurl('notification')">&#9654; curl command</button>
    <div class="curl-box" id="curl-notification"></div>
  </div>

  <!-- DASHBOARD -->
  <div class="panel" id="panel-dashboard">
    <div class="form-group"><label>Layout</label>
      <select id="dash-layout"><option value="grid">Grid</option></select>
    </div>
    <div style="display:flex;gap:8px;align-items:center;margin-bottom:10px">
      <strong style="font-size:13px">Widgets</strong>
      <button class="btn btn-secondary" onclick="addWidget()" style="padding:5px 10px;font-size:12px">+ Add</button>
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

  <!-- IMAGE -->
  <div class="panel" id="panel-image">
    <div class="form-group"><label>Upload Image (BMP format)</label>
      <input type="file" id="img-file" accept="image/*" style="padding:8px">
    </div>
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
        <div class="sys-item"><div class="label">WiFi Network</div><div class="value" id="sys-ssid">--</div></div>
        <div class="sys-item"><div class="label">Signal Strength</div><div class="value" id="sys-rssi">--</div></div>
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
            <option value="0">0 (Normal)</option>
            <option value="1">90</option>
            <option value="2">180</option>
            <option value="3">270</option>
          </select>
        </div>
      </div>
    </div>

    <div class="sys-section">
      <h3>Danger Zone</h3>
      <div class="danger-zone">
        <p>Reset WiFi credentials and reboot into setup mode. You will need to reconnect to the device's access point to reconfigure WiFi.</p>
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

// Toast notifications
function toast(msg, type) {
  const c = document.getElementById('toast-container');
  const t = document.createElement('div');
  t.className = 'toast ' + type;
  t.textContent = msg;
  c.appendChild(t);
  setTimeout(() => { t.style.opacity = '0'; t.style.transition = 'opacity .3s'; setTimeout(() => t.remove(), 300); }, 3000);
}

// Button loading state
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

// Toggle curl box
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
  } catch(e) {}
}

function showCurl(id, endpoint, data) {
  const el = document.getElementById('curl-' + id);
  if (!el) return;
  const host = location.hostname || 'epaper.local';
  const port = location.port ? ':' + location.port : '';
  const json = JSON.stringify(data);
  const cmd = "curl -X POST http://" + host + port + endpoint + " -H 'Content-Type: application/json' -d '" + json + "'";
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
  const r = await fetch(BASE + endpoint, {
    method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(data)
  });
  return r.json();
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
  return {layout: document.getElementById('dash-layout').value, widgets};
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

async function sendImage() {
  const file = document.getElementById('img-file').files[0];
  if (!file) { toast('No file selected', 'error'); return; }
  const fd = new FormData();
  fd.append('image', file);
  btnLoading('image', true);
  try {
    const r = await fetch(BASE + '/api/image', {method:'POST', body:fd});
    const d = await r.json();
    toast(d.ok ? 'Image displayed' : (d.error || 'Failed'), d.ok ? 'success' : 'error');
    if (d.ok) addHistory('image');
  } catch(e) { toast('Connection error', 'error'); }
  btnLoading('image', false);
}

async function clearDisplay() {
  try {
    const r = await apiPost('/api/clear', {});
    toast(r.ok ? 'Display cleared' : 'Failed', r.ok ? 'success' : 'error');
    if (r.ok) addHistory('clear');
  } catch(e) { toast('Connection error', 'error'); }
}

async function setRotation() {
  const rot = parseInt(document.getElementById('sys-rotation').value);
  try {
    const r = await apiPost('/api/config', {rotation: rot});
    toast(r.ok ? 'Rotation set to ' + (rot * 90) + ' degrees' : 'Failed', r.ok ? 'success' : 'error');
  } catch(e) { toast('Connection error', 'error'); }
}

async function resetWiFi() {
  if (!confirm('Reset WiFi credentials? The device will reboot into setup mode and you will lose connection.')) return;
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
