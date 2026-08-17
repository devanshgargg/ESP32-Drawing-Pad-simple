#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------- USER SETTINGS --------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* AP_SSID       = "DEVANSH_DISPLAY";
const char* AP_PASSWORD   = "12345678";     // must be 8+ characters

#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDR    0x3C     // change to 0x3D if your module doesn't show up
#define OLED_RESET   -1
// -----------------------------------------------------------------------------

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

volatile bool needsRender = false;
unsigned long lastRender = 0;
const unsigned long RENDER_INTERVAL_MS = 20;

void drawThickLine(int x0, int y0, int x1, int y1, int size) {
  x0 = constrain(x0, 0, OLED_WIDTH  - 1);
  x1 = constrain(x1, 0, OLED_WIDTH  - 1);
  y0 = constrain(y0, 0, OLED_HEIGHT - 1);
  y1 = constrain(y1, 0, OLED_HEIGHT - 1);

  if (size <= 1) {
    display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    needsRender = true;
    return;
  }

  int r = size / 2;
  int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
  int dy = -abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy, e2;

  while (true) {
    display.fillCircle(x0, y0, r, SSD1306_WHITE);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  needsRender = true;
}

void drawDot(int x, int y, int size) {
  x = constrain(x, 0, OLED_WIDTH  - 1);
  y = constrain(y, 0, OLED_HEIGHT - 1);
  int r = max(0, size / 2);
  display.fillCircle(x, y, r, SSD1306_WHITE);
  needsRender = true;
}

void clearPanel() {
  display.clearDisplay();
  needsRender = true;
}

void handleCommand(char* cmd) {
  char* type = strtok(cmd, ",");
  if (!type) return;

  if (strcmp(type, "CLR") == 0) {
    clearPanel();
  } else if (strcmp(type, "L") == 0) {
    char* a = strtok(NULL, ","); char* b = strtok(NULL, ",");
    char* c = strtok(NULL, ","); char* d = strtok(NULL, ",");
    char* e = strtok(NULL, ",");
    if (a && b && c && d && e) {
      drawThickLine(atoi(a), atoi(b), atoi(c), atoi(d), atoi(e));
    }
  } else if (strcmp(type, "P") == 0) {
    char* a = strtok(NULL, ","); char* b = strtok(NULL, ",");
    char* c = strtok(NULL, ",");
    if (a && b && c) {
      drawDot(atoi(a), atoi(b), atoi(c));
    }
  }
}


void handleMessage(uint8_t* data, size_t len) {
  static char buf[600];
  if (len >= sizeof(buf)) len = sizeof(buf) - 1;
  memcpy(buf, data, len);
  buf[len] = '\0';

  char* saveptr;
  char* token = strtok_r(buf, ";", &saveptr);
  while (token) {
    handleCommand(token);
    token = strtok_r(NULL, ";", &saveptr);
  }
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[ws] client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[ws] client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      handleMessage(data, len);
    }
  }
}

// ============================================================================
// WEB PAGE — all HTML / CSS / JS lives on the chip. No CDN, no internet
// required, so it works even when the phone is only on the ESP's hotspot.
// ============================================================================
const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>OLED Draw Pad</title>
<style>
  :root{
    --bg:#0a0d10;
    --panel:#12171c;
    --panel-2:#161c22;
    --border:#232b32;
    --accent:#6ee7d8;
    --accent-dim:#3a5a56;
    --warn:#ffb454;
    --text:#e6edf0;
    --text-dim:#7c8b94;
    --mono: ui-monospace, SFMono-Regular, Consolas, "Liberation Mono", Menlo, monospace;
    --sans: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  }
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent;}
  html,body{margin:0;height:100%;}
  body{
    background:
      radial-gradient(circle at 20% -10%, #10181c 0%, transparent 40%),
      var(--bg);
    color:var(--text);
    font-family:var(--sans);
    display:flex;
    flex-direction:column;
    align-items:center;
    min-height:100%;
    padding:20px 16px 32px;
    -webkit-user-select:none;
    user-select:none;
  }
  .topbar{
    width:100%;
    max-width:640px;
    display:flex;
    align-items:center;
    justify-content:space-between;
    margin-bottom:18px;
  }
  .brand{display:flex;align-items:baseline;gap:8px;font-family:var(--mono);letter-spacing:.5px;}
  .brand b{font-size:15px;font-weight:700;color:var(--text);}
  .brand span{font-size:11px;color:var(--text-dim);}
  .status{
    display:flex;align-items:center;gap:7px;
    font-family:var(--mono);font-size:11px;color:var(--text-dim);
    padding:5px 10px;border:1px solid var(--border);border-radius:20px;background:var(--panel);
  }
  .dot{width:7px;height:7px;border-radius:50%;background:#4a555c;transition:background .2s;}
  .status.live .dot{background:var(--accent);box-shadow:0 0 6px var(--accent);animation:pulse 1.6s infinite;}
  .status.live{color:var(--accent);}
  @keyframes pulse{0%,100%{opacity:1;}50%{opacity:.4;}}

  .device{
    width:100%;max-width:640px;
    background:linear-gradient(180deg,var(--panel) 0%,var(--panel-2) 100%);
    border:1px solid var(--border);border-radius:18px;
    padding:20px 20px 16px;
    box-shadow:0 20px 50px -20px rgba(0,0,0,.6), inset 0 1px 0 rgba(255,255,255,.02);
  }
  .screen-wrap{
    position:relative;border-radius:6px;background:#000;padding:14px;
    border:1px solid #1c2329;
    box-shadow:inset 0 0 0 1px rgba(255,255,255,.02), 0 0 40px -12px var(--accent-dim);
  }
  canvas#pad{
    display:block;width:100%;height:auto;aspect-ratio:2/1;
    background:#000;
    image-rendering:pixelated;
    image-rendering:crisp-edges;
    touch-action:none;
    cursor:crosshair;
    border-radius:2px;
  }
  .screen-wrap::after{
    content:"";position:absolute;inset:14px;pointer-events:none;
    background:repeating-linear-gradient(180deg, rgba(255,255,255,.025) 0px, rgba(255,255,255,.025) 1px, transparent 1px, transparent 3px);
    border-radius:2px;mix-blend-mode:screen;
  }
  .pinrow{display:flex;justify-content:center;gap:22px;margin-top:12px;font-family:var(--mono);font-size:9px;letter-spacing:1px;color:var(--text-dim);}
  .pinrow span{display:flex;align-items:center;gap:5px;}
  .pinrow i{width:5px;height:5px;border-radius:50%;background:var(--text-dim);display:block;}
  .label-tag{text-align:center;font-family:var(--mono);font-size:10px;color:var(--text-dim);letter-spacing:1.5px;margin-top:10px;text-transform:uppercase;}

  .controls{
    width:100%;max-width:640px;margin-top:16px;
    background:var(--panel);border:1px solid var(--border);border-radius:14px;
    padding:16px 18px;display:flex;flex-direction:column;gap:14px;
  }
  .row{display:flex;align-items:center;gap:14px;}
  .row label{font-family:var(--mono);font-size:11px;color:var(--text-dim);letter-spacing:1px;min-width:64px;}
  input[type=range]{flex:1;-webkit-appearance:none;height:4px;border-radius:2px;background:var(--border);outline:none;}
  input[type=range]::-webkit-slider-thumb{
    -webkit-appearance:none;width:20px;height:20px;border-radius:50%;
    background:var(--accent);border:3px solid #0a0d10;box-shadow:0 0 0 1px var(--accent);
    cursor:pointer;margin-top:-8px;
  }
  input[type=range]::-webkit-slider-runnable-track{height:4px;border-radius:2px;}
  input[type=range]::-moz-range-thumb{
    width:14px;height:14px;border-radius:50%;
    background:var(--accent);border:3px solid #0a0d10;box-shadow:0 0 0 1px var(--accent);cursor:pointer;
  }
  .pensize-val{font-family:var(--mono);font-size:13px;color:var(--text);min-width:34px;text-align:right;}
  .pen-preview{width:26px;height:26px;border-radius:50%;background:#000;border:1px solid var(--border);display:flex;align-items:center;justify-content:center;flex-shrink:0;}
  .pen-preview i{border-radius:50%;background:var(--accent);display:block;}

  .btnrow{display:flex;gap:10px;}
  button{
    font-family:var(--mono);font-size:12px;letter-spacing:1px;text-transform:uppercase;
    border-radius:9px;border:1px solid var(--border);padding:12px 16px;cursor:pointer;
    transition:transform .08s, background .15s, border-color .15s;
  }
  button:active{transform:scale(.96);}
  .btn-clear{flex:1;background:transparent;color:var(--warn);border-color:#4a3a24;}
  .btn-clear:active{background:rgba(255,180,84,.08);}

  footer{margin-top:18px;font-family:var(--mono);font-size:10px;color:#4a555c;letter-spacing:.5px;}
</style>
</head>
<body>

  <div class="topbar">
    <div class="brand"><b>DRAW&middot;PAD</b><span>SSD1306 &middot; 128&times;64</span></div>
    <div class="status" id="status"><span class="dot"></span><span id="statusText">connecting</span></div>
  </div>

  <div class="device">
    <div class="screen-wrap">
      <canvas id="pad" width="128" height="64"></canvas>
    </div>
    <div class="pinrow">
      <span><i></i>VCC</span>
      <span><i></i>GND</span>
      <span><i></i>SDA</span>
      <span><i></i>SCL</span>
    </div>
    <div class="label-tag">live mirror &mdash; tech by nandhu</div>
  </div>

  <div class="controls">
    <div class="row">
      <label>PEN SIZE</label>
      <input type="range" id="penSize" min="1" max="8" value="2" step="1">
      <div class="pen-preview"><i id="penDot"></i></div>
      <div class="pensize-val" id="penVal">2px</div>
    </div>
    <div class="btnrow">
      <button class="btn-clear" id="clearBtn">Clear panel</button>
    </div>
  </div>

  <footer>ESP8266 &middot; WebSocket live draw &middot; no-lag mode</footer>

<script>
(function(){
  const canvas = document.getElementById('pad');
  const ctx = canvas.getContext('2d');
  ctx.fillStyle = '#000';
  ctx.fillRect(0,0,canvas.width,canvas.height);

  const statusEl = document.getElementById('status');
  const statusText = document.getElementById('statusText');
  const penSizeInput = document.getElementById('penSize');
  const penVal = document.getElementById('penVal');
  const penDot = document.getElementById('penDot');
  const clearBtn = document.getElementById('clearBtn');

  let penSize = parseInt(penSizeInput.value, 10);

  function updatePenPreview(){
    penVal.textContent = penSize + 'px';
    const px = Math.max(2, Math.min(18, penSize * 2));
    penDot.style.width = px + 'px';
    penDot.style.height = px + 'px';
  }
  updatePenPreview();

  penSizeInput.addEventListener('input', () => {
    penSize = parseInt(penSizeInput.value, 10);
    updatePenPreview();
  });

  // ---------------- WebSocket, with auto-reconnect ----------------
  let ws = null;
  let wsReady = false;
  let reconnectTimer = null;

  function setStatus(connected, text){
    statusEl.classList.toggle('live', connected);
    statusText.textContent = text;
  }

  function connect(){
    setStatus(false, 'connecting');
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(proto + '://' + location.host + '/ws');
    ws.onopen = () => { wsReady = true; setStatus(true, 'live'); };
    ws.onclose = () => {
      wsReady = false;
      setStatus(false, 'reconnecting');
      clearTimeout(reconnectTimer);
      reconnectTimer = setTimeout(connect, 1200);
    };
    ws.onerror = () => { try{ ws.close(); }catch(e){} };
  }
  connect();

  // ------- batched sender: coalesces fast pointermove into one WS frame -------
  let queue = [];
  function queueSend(cmd){ queue.push(cmd); }
  function flush(){
    if (queue.length && wsReady && ws.readyState === WebSocket.OPEN) {
      ws.send(queue.join(';'));
      queue = [];
    }
    requestAnimationFrame(flush);
  }
  requestAnimationFrame(flush);
  function flushNow(){
    if (queue.length && wsReady && ws.readyState === WebSocket.OPEN) {
      ws.send(queue.join(';'));
      queue = [];
    }
  }

  // ---------------------------- drawing ----------------------------
  let drawing = false;
  let lastX = 0, lastY = 0;

  function toCanvasPoint(clientX, clientY){
    const rect = canvas.getBoundingClientRect();
    let x = Math.round((clientX - rect.left) / rect.width  * canvas.width);
    let y = Math.round((clientY - rect.top)  / rect.height * canvas.height);
    x = Math.max(0, Math.min(canvas.width  - 1, x));
    y = Math.max(0, Math.min(canvas.height - 1, y));
    return [x, y];
  }

  function localDot(x, y, size){
    ctx.fillStyle = '#fff';
    ctx.beginPath();
    ctx.arc(x, y, Math.max(0.5, size/2), 0, Math.PI*2);
    ctx.fill();
  }
  function localLine(x0,y0,x1,y1,size){
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = size;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    ctx.moveTo(x0,y0);
    ctx.lineTo(x1,y1);
    ctx.stroke();
    localDot(x1,y1,size);
  }

  function pointerDown(e){
    canvas.setPointerCapture(e.pointerId);
    const [x,y] = toCanvasPoint(e.clientX, e.clientY);
    drawing = true;
    lastX = x; lastY = y;
    localDot(x, y, penSize);
    queueSend('P,' + x + ',' + y + ',' + penSize);
  }
  function pointerMove(e){
    if (!drawing) return;
    const [x,y] = toCanvasPoint(e.clientX, e.clientY);
    if (x === lastX && y === lastY) return;
    localLine(lastX, lastY, x, y, penSize);
    queueSend('L,' + lastX + ',' + lastY + ',' + x + ',' + y + ',' + penSize);
    lastX = x; lastY = y;
  }
  function pointerUp(e){
    if (!drawing) return;
    drawing = false;
    try{ canvas.releasePointerCapture(e.pointerId); }catch(err){}
  }

  canvas.addEventListener('pointerdown', pointerDown);
  canvas.addEventListener('pointermove', pointerMove);
  canvas.addEventListener('pointerup', pointerUp);
  canvas.addEventListener('pointercancel', pointerUp);
  canvas.addEventListener('pointerleave', (e)=>{ if(drawing) pointerUp(e); });
  canvas.addEventListener('contextmenu', (e)=> e.preventDefault());

  clearBtn.addEventListener('click', () => {
    ctx.fillStyle = '#000';
    ctx.fillRect(0,0,canvas.width,canvas.height);
    queue = ['CLR'];
    flushNow();
  });
})();
</script>
</body>
</html>
)HTMLPAGE";

void setup() {
  Serial.begin(115200);
  delay(200);
  
  #ifdef ESP8266
  Wire.begin(4, 5);       // SDA = GPIO4 (D2), SCL = GPIO5 (D1)
#elif defined(ESP32)
  Wire.begin(21, 22);     // SDA = GPIO21, SCL = GPIO22
#endif
Wire.setClock(400000);   // fast I2C -> lower render latency

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 not found - check wiring / I2C address");
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("OLED Draw Pad");
  display.println("connecting wifi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 10000) {
    if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
    delay(250);
  }

  IPAddress ip;
  if (connected) {
    ip = WiFi.localIP();
    Serial.print("Connected. Open: http://");
    Serial.println(ip);
  } else {
    Serial.println("WiFi failed - starting access point");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    ip = WiFi.softAPIP();
    Serial.print("AP started. Open: http://");
    Serial.println(ip);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  if (connected) {
    display.println("WiFi connected");
  } else {
    display.println("Hotspot:");
    display.println(AP_SSID);
  }
  display.println("Open in browser:");
  display.println(ip);
  display.display();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();
  unsigned long now = millis();
  if (needsRender && (now - lastRender >= RENDER_INTERVAL_MS)) {
    display.display();
    needsRender = false;
    lastRender = now;
  }
}
