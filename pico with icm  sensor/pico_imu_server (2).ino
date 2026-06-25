/*
  CubeSat Control Panel — Self-Hosted IMU Server
  Raspberry Pi Pico 2W + ICM-20948

  Opens your browser to:  http://<pico-ip>/
  Sensor JSON at:         http://<pico-ip>/data

  Wiring:
    ICM-20948 SDA → GP0
    ICM-20948 SCL → GP1
    ICM-20948 VCC → 3V3
    ICM-20948 GND → GND
    ICM-20948 AD0 → GND  (addr 0x68)
*/

#include <Wire.h>
#include <math.h>
#include <WiFi.h>

// ─── WiFi credentials ────────────────────────────────────────────────────────
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ─── ICM-20948 ───────────────────────────────────────────────────────────────
#define ICM_ADDR 0x68

int16_t ax, ay, az;
float rollFiltered  = 0;
float pitchFiltered = 0;
const float alpha   = 0.25;

// ─── Server ──────────────────────────────────────────────────────────────────
WiFiServer server(80);

// ─── CORS (needed for /data endpoint) ────────────────────────────────────────
void sendCORS(WiFiClient& client) {
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
}

// ─── ICM register helpers ────────────────────────────────────────────────────
void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}
uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}
void readAccel() {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(0x2D);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, 6);
  if (Wire.available() >= 6) {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();
  }
}

// ─── Dashboard HTML (auto-generated) ─────────────────────────────────────────
// Auto-generated HTML page — served at http://<pico-ip>/
void sendDashboard(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("<meta charset=\"UTF-8\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("<title>CubeSat Control Panel</title>");
  client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.js\"></script>");
  client.println("<style>");
  client.println("  :root {");
  client.println("    --bg:       #0d1117;");
  client.println("    --surface:  #161b22;");
  client.println("    --surface2: #21262d;");
  client.println("    --border:   #30363d;");
  client.println("    --text:     #e6edf3;");
  client.println("    --muted:    #8b949e;");
  client.println("    --accent:   #388bfd;");
  client.println("    --green:    #3fb950;");
  client.println("    --yellow:   #d29922;");
  client.println("    --red:      #f85149;");
  client.println("    --teal:     #39d353;");
  client.println("  }");
  client.println("  * { box-sizing: border-box; margin: 0; padding: 0; }");
  client.println("  body { background: var(--bg); color: var(--text); font-family: 'Segoe UI', system-ui, sans-serif; font-size: 14px; min-height: 100vh; }");
  client.println("");
  client.println("  header {");
  client.println("    background: var(--surface);");
  client.println("    border-bottom: 1px solid var(--border);");
  client.println("    padding: 12px 24px;");
  client.println("    display: flex;");
  client.println("    align-items: center;");
  client.println("    justify-content: space-between;");
  client.println("  }");
  client.println("  .logo { display: flex; align-items: center; gap: 10px; }");
  client.println("  .logo svg { width: 28px; height: 28px; }");
  client.println("  .logo-text { font-size: 16px; font-weight: 600; letter-spacing: .5px; }");
  client.println("  .logo-sub { font-size: 11px; color: var(--muted); }");
  client.println("  .header-right { display: flex; align-items: center; gap: 12px; }");
  client.println("");
  client.println("  .conn-bar {");
  client.println("    background: var(--surface);");
  client.println("    border-bottom: 1px solid var(--border);");
  client.println("    padding: 10px 24px;");
  client.println("    display: flex;");
  client.println("    align-items: center;");
  client.println("    gap: 10px;");
  client.println("    flex-wrap: wrap;");
  client.println("  }");
  client.println("  .conn-bar label { color: var(--muted); font-size: 12px; }");
  client.println("  .conn-bar input {");
  client.println("    background: var(--surface2);");
  client.println("    border: 1px solid var(--border);");
  client.println("    color: var(--text);");
  client.println("    border-radius: 6px;");
  client.println("    padding: 5px 10px;");
  client.println("    font-size: 13px;");
  client.println("    width: 180px;");
  client.println("    outline: none;");
  client.println("  }");
  client.println("  .conn-bar input:focus { border-color: var(--accent); }");
  client.println("  .btn {");
  client.println("    padding: 5px 14px;");
  client.println("    border-radius: 6px;");
  client.println("    border: 1px solid var(--border);");
  client.println("    background: var(--surface2);");
  client.println("    color: var(--text);");
  client.println("    cursor: pointer;");
  client.println("    font-size: 13px;");
  client.println("    transition: background .15s;");
  client.println("  }");
  client.println("  .btn:hover { background: #2d333b; }");
  client.println("  .btn.primary { background: var(--accent); border-color: var(--accent); color: #fff; }");
  client.println("  .btn.primary:hover { background: #58a6ff; }");
  client.println("");
  client.println("  .pill {");
  client.println("    display: inline-flex;");
  client.println("    align-items: center;");
  client.println("    gap: 5px;");
  client.println("    font-size: 11px;");
  client.println("    padding: 3px 10px;");
  client.println("    border-radius: 20px;");
  client.println("    border: 1px solid var(--border);");
  client.println("    background: var(--surface2);");
  client.println("    color: var(--muted);");
  client.println("  }");
  client.println("  .pill.live { border-color: var(--green); color: var(--green); }");
  client.println("  .pill.error { border-color: var(--red); color: var(--red); }");
  client.println("  .dot { width: 7px; height: 7px; border-radius: 50%; background: currentColor; }");
  client.println("  .dot.pulse { animation: pulse 1.2s infinite; }");
  client.println("  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.3} }");
  client.println("");
  client.println("  main { padding: 20px 24px; max-width: 1100px; margin: 0 auto; }");
  client.println("");
  client.println("  .cards {");
  client.println("    display: grid;");
  client.println("    grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));");
  client.println("    gap: 12px;");
  client.println("    margin-bottom: 20px;");
  client.println("  }");
  client.println("  .card {");
  client.println("    background: var(--surface);");
  client.println("    border: 1px solid var(--border);");
  client.println("    border-radius: 10px;");
  client.println("    padding: 14px 16px;");
  client.println("  }");
  client.println("  .card .lbl { font-size: 11px; color: var(--muted); text-transform: uppercase; letter-spacing: .6px; margin-bottom: 6px; }");
  client.println("  .card .val { font-size: 26px; font-weight: 600; font-variant-numeric: tabular-nums; line-height: 1; }");
  client.println("  .card .unit { font-size: 12px; color: var(--muted); margin-left: 3px; }");
  client.println("  .card.highlight { border-color: var(--accent); }");
  client.println("  #card-roll .val  { color: #58a6ff; }");
  client.println("  #card-pitch .val { color: #f778ba; }");
  client.println("");
  client.println("  .charts {");
  client.println("    display: grid;");
  client.println("    grid-template-columns: 1fr 1fr;");
  client.println("    gap: 14px;");
  client.println("    margin-bottom: 20px;");
  client.println("  }");
  client.println("  @media (max-width: 640px) { .charts { grid-template-columns: 1fr; } }");
  client.println("");
  client.println("  .chart-panel {");
  client.println("    background: var(--surface);");
  client.println("    border: 1px solid var(--border);");
  client.println("    border-radius: 10px;");
  client.println("    padding: 14px 16px;");
  client.println("  }");
  client.println("  .chart-panel h3 { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .6px; margin-bottom: 10px; }");
  client.println("  .legend { display: flex; gap: 14px; font-size: 11px; color: var(--muted); margin-bottom: 8px; flex-wrap: wrap; }");
  client.println("  .legend span { display: flex; align-items: center; gap: 4px; }");
  client.println("  .leg { width: 10px; height: 3px; border-radius: 2px; }");
  client.println("  .chart-wrap { position: relative; height: 160px; }");
  client.println("");
  client.println("  .attitude-panel {");
  client.println("    background: var(--surface);");
  client.println("    border: 1px solid var(--border);");
  client.println("    border-radius: 10px;");
  client.println("    padding: 14px 16px;");
  client.println("    margin-bottom: 20px;");
  client.println("    display: flex;");
  client.println("    gap: 24px;");
  client.println("    align-items: center;");
  client.println("    flex-wrap: wrap;");
  client.println("  }");
  client.println("  .attitude-panel h3 { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .6px; margin-bottom: 10px; width: 100%; }");
  client.println("  .gauge-wrap { display: flex; flex-direction: column; align-items: center; gap: 6px; flex: 1; min-width: 120px; }");
  client.println("  .gauge-wrap .g-lbl { font-size: 11px; color: var(--muted); }");
  client.println("  .gauge-bar { width: 100%; height: 10px; background: var(--surface2); border-radius: 5px; overflow: hidden; border: 1px solid var(--border); position: relative; }");
  client.println("  .gauge-fill { height: 100%; border-radius: 5px; transition: width .3s, background .3s; }");
  client.println("  .gauge-val { font-size: 18px; font-weight: 600; font-variant-numeric: tabular-nums; }");
  client.println("");
  client.println("  .log {");
  client.println("    background: var(--surface);");
  client.println("    border: 1px solid var(--border);");
  client.println("    border-radius: 10px;");
  client.println("    padding: 14px 16px;");
  client.println("  }");
  client.println("  .log h3 { font-size: 12px; color: var(--muted); text-transform: uppercase; letter-spacing: .6px; margin-bottom: 10px; }");
  client.println("  .log-entries { font-family: 'Courier New', monospace; font-size: 12px; color: var(--muted); height: 80px; overflow-y: auto; }");
  client.println("  .log-entries .entry { padding: 1px 0; }");
  client.println("  .log-entries .entry .ts { color: #3fb950; margin-right: 8px; }");
  client.println("  .log-entries .entry.err { color: var(--red); }");
  client.println("");
  client.println("  footer { text-align: center; padding: 16px; font-size: 11px; color: var(--muted); border-top: 1px solid var(--border); margin-top: 8px; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("");
  client.println("<header>");
  client.println("  <div class=\"logo\">");
  client.println("    <svg viewBox=\"0 0 28 28\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\">");
  client.println("      <rect x=\"9\" y=\"9\" width=\"10\" height=\"10\" rx=\"2\" stroke=\"#388bfd\" stroke-width=\"1.5\"/>");
  client.println("      <line x1=\"4\" y1=\"14\" x2=\"9\" y2=\"14\" stroke=\"#388bfd\" stroke-width=\"1.5\"/>");
  client.println("      <line x1=\"19\" y1=\"14\" x2=\"24\" y2=\"14\" stroke=\"#388bfd\" stroke-width=\"1.5\"/>");
  client.println("      <line x1=\"14\" y1=\"4\" x2=\"14\" y2=\"9\" stroke=\"#388bfd\" stroke-width=\"1.5\"/>");
  client.println("      <line x1=\"14\" y1=\"19\" x2=\"14\" y2=\"24\" stroke=\"#388bfd\" stroke-width=\"1.5\"/>");
  client.println("      <rect x=\"2\" y=\"12\" width=\"4\" height=\"4\" rx=\"1\" fill=\"#388bfd\" opacity=\".7\"/>");
  client.println("      <rect x=\"22\" y=\"12\" width=\"4\" height=\"4\" rx=\"1\" fill=\"#388bfd\" opacity=\".7\"/>");
  client.println("      <rect x=\"12\" y=\"2\" width=\"4\" height=\"4\" rx=\"1\" fill=\"#388bfd\" opacity=\".7\"/>");
  client.println("      <rect x=\"12\" y=\"22\" width=\"4\" height=\"4\" rx=\"1\" fill=\"#388bfd\" opacity=\".7\"/>");
  client.println("    </svg>");
  client.println("    <div>");
  client.println("      <div class=\"logo-text\">CubeSat Control Panel</div>");
  client.println("      <div class=\"logo-sub\">IMU Subsystem — ICM-20948</div>");
  client.println("    </div>");
  client.println("  </div>");
  client.println("  <div class=\"header-right\">");
  client.println("    <span class=\"pill\" id=\"status-pill\"><span class=\"dot\" id=\"status-dot\"></span><span id=\"status-text\">Disconnected</span></span>");
  client.println("    <span style=\"font-size:11px;color:var(--muted)\" id=\"rate-display\"></span>");
  client.println("  </div>");
  client.println("</header>");
  client.println("");
  client.println("<div class=\"conn-bar\">");
  client.println("  <label>Pico IP address:</label>");
  client.println("  <input type=\"text\" id=\"ip-input\" placeholder=\"192.168.1.xxx\" value=\"192.168.1.100\" />");
  client.println("  <button class=\"btn primary\" id=\"conn-btn\">Connect</button>");
  client.println("  <button class=\"btn\" id=\"clear-btn\">Clear charts</button>");
  client.println("  <span style=\"font-size:11px;color:var(--muted);margin-left:4px\">Poll interval: 500ms &nbsp;|&nbsp; Endpoint: <code style=\"color:var(--accent)\">/data</code></span>");
  client.println("</div>");
  client.println("");
  client.println("<main>");
  client.println("  <div class=\"cards\">");
  client.println("    <div class=\"card highlight\" id=\"card-roll\">");
  client.println("      <div class=\"lbl\">Roll</div>");
  client.println("      <div><span class=\"val\" id=\"v-roll\">—</span><span class=\"unit\">°</span></div>");
  client.println("    </div>");
  client.println("    <div class=\"card highlight\" id=\"card-pitch\">");
  client.println("      <div class=\"lbl\">Pitch</div>");
  client.println("      <div><span class=\"val\" id=\"v-pitch\">—</span><span class=\"unit\">°</span></div>");
  client.println("    </div>");
  client.println("    <div class=\"card\">");
  client.println("      <div class=\"lbl\">Accel X</div>");
  client.println("      <div><span class=\"val\" id=\"v-ax\" style=\"font-size:20px\">—</span><span class=\"unit\">raw</span></div>");
  client.println("    </div>");
  client.println("    <div class=\"card\">");
  client.println("      <div class=\"lbl\">Accel Y</div>");
  client.println("      <div><span class=\"val\" id=\"v-ay\" style=\"font-size:20px\">—</span><span class=\"unit\">raw</span></div>");
  client.println("    </div>");
  client.println("    <div class=\"card\">");
  client.println("      <div class=\"lbl\">Accel Z</div>");
  client.println("      <div><span class=\"val\" id=\"v-az\" style=\"font-size:20px\">—</span><span class=\"unit\">raw</span></div>");
  client.println("    </div>");
  client.println("    <div class=\"card\">");
  client.println("      <div class=\"lbl\">Uptime</div>");
  client.println("      <div><span class=\"val\" id=\"v-uptime\" style=\"font-size:18px\">—</span><span class=\"unit\">s</span></div>");
  client.println("    </div>");
  client.println("  </div>");
  client.println("");
  client.println("  <div class=\"attitude-panel\">");
  client.println("    <h3>Attitude indicator</h3>");
  client.println("    <div class=\"gauge-wrap\">");
  client.println("      <div class=\"g-lbl\">Roll</div>");
  client.println("      <div class=\"gauge-bar\"><div class=\"gauge-fill\" id=\"g-roll\" style=\"width:50%;background:#388bfd\"></div></div>");
  client.println("      <div class=\"gauge-val\" id=\"gv-roll\" style=\"color:#58a6ff\">0.0°</div>");
  client.println("    </div>");
  client.println("    <div class=\"gauge-wrap\">");
  client.println("      <div class=\"g-lbl\">Pitch</div>");
  client.println("      <div class=\"gauge-bar\"><div class=\"gauge-fill\" id=\"g-pitch\" style=\"width:50%;background:#d2a8ff\"></div></div>");
  client.println("      <div class=\"gauge-val\" id=\"gv-pitch\" style=\"color:#f778ba\">0.0°</div>");
  client.println("    </div>");
  client.println("  </div>");
  client.println("");
  client.println("  <div class=\"charts\">");
  client.println("    <div class=\"chart-panel\">");
  client.println("      <h3>Roll &amp; Pitch</h3>");
  client.println("      <div class=\"legend\">");
  client.println("        <span><span class=\"leg\" style=\"background:#388bfd\"></span>Roll</span>");
  client.println("        <span><span class=\"leg\" style=\"background:#f778ba;border-top:2px dashed #f778ba;background:none;width:16px\"></span>Pitch</span>");
  client.println("      </div>");
  client.println("      <div class=\"chart-wrap\"><canvas id=\"angleChart\" role=\"img\" aria-label=\"Live roll and pitch angle chart\"></canvas></div>");
  client.println("    </div>");
  client.println("    <div class=\"chart-panel\">");
  client.println("      <h3>Accelerometer</h3>");
  client.println("      <div class=\"legend\">");
  client.println("        <span><span class=\"leg\" style=\"background:#3fb950\"></span>X</span>");
  client.println("        <span><span class=\"leg\" style=\"background:#d29922\"></span>Y</span>");
  client.println("        <span><span class=\"leg\" style=\"background:#f85149\"></span>Z</span>");
  client.println("      </div>");
  client.println("      <div class=\"chart-wrap\"><canvas id=\"accelChart\" role=\"img\" aria-label=\"Live accelerometer X Y Z chart\"></canvas></div>");
  client.println("    </div>");
  client.println("  </div>");
  client.println("");
  client.println("  <div class=\"log\">");
  client.println("    <h3>System log</h3>");
  client.println("    <div class=\"log-entries\" id=\"log-entries\"></div>");
  client.println("  </div>");
  client.println("</main>");
  client.println("");
  client.println("<footer>CubeSat Ground Control · IMU Subsystem · Pico 2W + ICM-20948</footer>");
  client.println("");
  client.println("<script>");
  client.println("const MAX = 80;");
  client.println("const labels = [];");
  client.println("let count = 0, pollTimer = null, lastTime = null;");
  client.println("");
  client.println("const mkDataset = (label, color, dash=[]) => ({");
  client.println("  label, data: [],");
  client.println("  borderColor: color, backgroundColor: 'transparent',");
  client.println("  borderWidth: 1.5, pointRadius: 0, tension: 0.3,");
  client.println("  borderDash: dash");
  client.println("});");
  client.println("");
  client.println("const angleData = {");
  client.println("  labels,");
  client.println("  datasets: [");
  client.println("    mkDataset('Roll',  '#388bfd'),");
  client.println("    mkDataset('Pitch', '#f778ba', [5,3])");
  client.println("  ]");
  client.println("};");
  client.println("const accelData = {");
  client.println("  labels,");
  client.println("  datasets: [");
  client.println("    mkDataset('X', '#3fb950'),");
  client.println("    mkDataset('Y', '#d29922', [4,3]),");
  client.println("    mkDataset('Z', '#f85149', [2,2])");
  client.println("  ]");
  client.println("};");
  client.println("");
  client.println("const chartOpts = (yLabel) => ({");
  client.println("  responsive: true, maintainAspectRatio: false,");
  client.println("  animation: { duration: 0 },");
  client.println("  plugins: { legend: { display: false } },");
  client.println("  scales: {");
  client.println("    x: { display: false },");
  client.println("    y: {");
  client.println("      grid: { color: 'rgba(255,255,255,.06)' },");
  client.println("      ticks: { font: { size: 10 }, color: '#8b949e' },");
  client.println("      title: { display: true, text: yLabel, font: { size: 10 }, color: '#8b949e' }");
  client.println("    }");
  client.println("  }");
  client.println("});");
  client.println("");
  client.println("const angleChart = new Chart(document.getElementById('angleChart'), { type:'line', data:angleData, options:chartOpts('degrees') });");
  client.println("const accelChart = new Chart(document.getElementById('accelChart'), { type:'line', data:accelData, options:chartOpts('raw counts') });");
  client.println("");
  client.println("function update(d) {");
  client.println("  count++;");
  client.println("  labels.push(count);");
  client.println("  angleData.datasets[0].data.push(d.roll);");
  client.println("  angleData.datasets[1].data.push(d.pitch);");
  client.println("  accelData.datasets[0].data.push(d.ax);");
  client.println("  accelData.datasets[1].data.push(d.ay);");
  client.println("  accelData.datasets[2].data.push(d.az);");
  client.println("  if (labels.length > MAX) {");
  client.println("    labels.shift();");
  client.println("    angleData.datasets.forEach(s=>s.data.shift());");
  client.println("    accelData.datasets.forEach(s=>s.data.shift());");
  client.println("  }");
  client.println("  angleChart.update('none');");
  client.println("  accelChart.update('none');");
  client.println("");
  client.println("  document.getElementById('v-roll').textContent  = d.roll.toFixed(1);");
  client.println("  document.getElementById('v-pitch').textContent = d.pitch.toFixed(1);");
  client.println("  document.getElementById('v-ax').textContent = d.ax;");
  client.println("  document.getElementById('v-ay').textContent = d.ay;");
  client.println("  document.getElementById('v-az').textContent = d.az;");
  client.println("  document.getElementById('v-uptime').textContent = (d.uptime/1000).toFixed(0);");
  client.println("");
  client.println("  // Gauges — map ±90° to 0-100%");
  client.println("  const rPct = Math.min(100, Math.max(0, (d.roll + 90) / 180 * 100));");
  client.println("  const pPct = Math.min(100, Math.max(0, (d.pitch + 90) / 180 * 100));");
  client.println("  document.getElementById('g-roll').style.width  = rPct + '%';");
  client.println("  document.getElementById('g-pitch').style.width = pPct + '%';");
  client.println("  document.getElementById('gv-roll').textContent  = d.roll.toFixed(1) + '°';");
  client.println("  document.getElementById('gv-pitch').textContent = d.pitch.toFixed(1) + '°';");
  client.println("");
  client.println("  // Color gauge by magnitude");
  client.println("  const rollColor  = Math.abs(d.roll)  > 45 ? '#f85149' : Math.abs(d.roll)  > 20 ? '#d29922' : '#388bfd';");
  client.println("  const pitchColor = Math.abs(d.pitch) > 45 ? '#f85149' : Math.abs(d.pitch) > 20 ? '#d29922' : '#d2a8ff';");
  client.println("  document.getElementById('g-roll').style.background  = rollColor;");
  client.println("  document.getElementById('g-pitch').style.background = pitchColor;");
  client.println("");
  client.println("  // Rate");
  client.println("  const now = Date.now();");
  client.println("  if (lastTime) {");
  client.println("    const hz = (1000 / (now - lastTime)).toFixed(1);");
  client.println("    document.getElementById('rate-display').textContent = hz + ' Hz';");
  client.println("  }");
  client.println("  lastTime = now;");
  client.println("}");
  client.println("");
  client.println("function log(msg, isErr=false) {");
  client.println("  const el = document.getElementById('log-entries');");
  client.println("  const ts = new Date().toTimeString().slice(0,8);");
  client.println("  const div = document.createElement('div');");
  client.println("  div.className = 'entry' + (isErr ? ' err' : '');");
  client.println("  div.innerHTML = `<span class=\"ts\">${ts}</span>${msg}`;");
  client.println("  el.appendChild(div);");
  client.println("  el.scrollTop = el.scrollHeight;");
  client.println("  if (el.children.length > 60) el.removeChild(el.firstChild);");
  client.println("}");
  client.println("");
  client.println("function setStatus(state) {");
  client.println("  const pill = document.getElementById('status-pill');");
  client.println("  const dot  = document.getElementById('status-dot');");
  client.println("  const txt  = document.getElementById('status-text');");
  client.println("  pill.className = 'pill ' + state;");
  client.println("  dot.className  = 'dot' + (state === 'live' ? ' pulse' : '');");
  client.println("  txt.textContent = state === 'live' ? 'Live' : state === 'error' ? 'Error' : 'Disconnected';");
  client.println("}");
  client.println("");
  client.println("let connected = false;");
  client.println("");
  client.println("function getIP() {");
  client.println("  return document.getElementById('ip-input').value.trim();");
  client.println("}");
  client.println("");
  client.println("async function poll() {");
  client.println("  const url = `http://${window.location.hostname}/data`;");
  client.println("  try {");
  client.println("    const res = await fetch(url, { signal: AbortSignal.timeout(1500) });");
  client.println("    if (!res.ok) throw new Error('HTTP ' + res.status);");
  client.println("    const d = await res.json();");
  client.println("    update(d);");
  client.println("    setStatus('live');");
  client.println("  } catch(e) {");
  client.println("    setStatus('error');");
  client.println("    log('Fetch failed: ' + e.message, true);");
  client.println("  }");
  client.println("}");
  client.println("");
  client.println("function startPolling() {");
  client.println("  if (pollTimer) clearInterval(pollTimer);");
  client.println("  log('Connecting to ' + getIP() + '...');");
  client.println("  poll();");
  client.println("  pollTimer = setInterval(poll, 500);");
  client.println("  connected = true;");
  client.println("  document.getElementById('conn-btn').textContent = 'Disconnect';");
  client.println("}");
  client.println("");
  client.println("function stopPolling() {");
  client.println("  clearInterval(pollTimer);");
  client.println("  pollTimer = null;");
  client.println("  connected = false;");
  client.println("  setStatus('');");
  client.println("  document.getElementById('conn-btn').textContent = 'Connect';");
  client.println("  document.getElementById('rate-display').textContent = '';");
  client.println("  log('Disconnected.');");
  client.println("}");
  client.println("");
  client.println("document.getElementById('conn-btn').onclick = () => {");
  client.println("  connected ? stopPolling() : startPolling();");
  client.println("};");
  client.println("");
  client.println("document.getElementById('clear-btn').onclick = () => {");
  client.println("  labels.length = 0; count = 0; lastTime = null;");
  client.println("  angleData.datasets.forEach(d=>d.data=[]);");
  client.println("  accelData.datasets.forEach(d=>d.data=[]);");
  client.println("  angleChart.update(); accelChart.update();");
  client.println("  ['v-roll','v-pitch','v-ax','v-ay','v-az','v-uptime'].forEach(id=>document.getElementById(id).textContent='—');");
  client.println("  log('Charts cleared.');");
  client.println("};");
  client.println("");
  client.println("document.getElementById('ip-input').addEventListener('keydown', e => {");
  client.println("  if (e.key === 'Enter' && !connected) startPolling();");
  client.println("});");
  client.println("");
  client.println("log('Auto-connecting to Pico...');");
  client.println("startPolling();");
  client.println("</script>");
  client.println("</body>");
  client.println("</html>");
  client.println("");
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.setSDA(0); Wire.setSCL(1);
  Wire.begin();
  delay(200);

  uint8_t whoami = readReg(0x00);
  Serial.print("WHO_AM_I = 0x"); Serial.println(whoami, HEX);
  if (whoami != 0xEA) {
    Serial.println("ICM-20948 NOT FOUND"); while(1);
  }
  writeReg(0x06, 0x01);
  Serial.println("ICM-20948 OK");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print(">>> Open browser at: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  readAccel();
  float roll  = atan2((float)ay, (float)az) * 57.2958;
  float pitch = atan2(-(float)ax, sqrt((float)ay*ay + (float)az*az)) * 57.2958;
  rollFiltered  = rollFiltered  * (1.0 - alpha) + roll  * alpha;
  pitchFiltered = pitchFiltered * (1.0 - alpha) + pitch * alpha;

  WiFiClient client = server.accept();
  if (!client) return;

  unsigned long t = millis();
  while (!client.available() && millis() - t < 500);
  String req = client.readStringUntil('\r');
  client.readString();

  // OPTIONS preflight
  if (req.indexOf("OPTIONS") >= 0) {
    client.println("HTTP/1.1 204 No Content");
    sendCORS(client);
    client.println("Connection: close");
    client.println();
    client.stop(); return;
  }

  // GET /data  → JSON
  if (req.indexOf("GET /data") >= 0) {
    String json = "{\"roll\":" + String(rollFiltered, 2)
                + ",\"pitch\":" + String(pitchFiltered, 2)
                + ",\"ax\":" + String(ax)
                + ",\"ay\":" + String(ay)
                + ",\"az\":" + String(az)
                + ",\"uptime\":" + String(millis()) + "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    sendCORS(client);
    client.println("Connection: close");
    client.println();
    client.print(json);

  // GET /  → Dashboard HTML
  } else if (req.indexOf("GET /") >= 0) {
    sendDashboard(client);

  } else {
    client.println("HTTP/1.1 404 Not Found");
    sendCORS(client);
    client.println("Connection: close");
    client.println();
  }

  client.stop();
}
