# ESP32 WiFi Test Tool

An 802.11 test harness for the ESP32 that turns the board into a self-contained WiFi security testing device with a mobile-friendly web control panel. Built with PlatformIO + the Arduino framework for the `esp32dev` board.

> ⚠️ **Authorized testing only.** This tool transmits 802.11 deauthentication and beacon management frames. Use it exclusively on networks you own or have written permission to test. Deauthing networks you don't control is illegal in most jurisdictions.

## What it does

The ESP32 hosts its own WiFi access point and web server, so you control everything from a phone or laptop browser — no cables, no serial console, no extra software.

**1. Network scanner**
Scans for nearby WiFi networks and lists them with SSID, BSSID, channel, and signal strength (rendered as live signal bars). Tap any network to select it as a target. Optional auto-refresh every 8 seconds.

**2. Deauthentication attack**
Sends broadcast 802.11 deauth frames spoofed from the selected AP's BSSID, forcing clients on that network to disconnect. Runs continuously until stopped, with a live packet counter.

**3. Beacon spam**
Broadcasts up to 20 fake access points from a user-supplied list of SSIDs (one per line), each with a randomized locally-administered MAC address, flooding the airwaves with bogus network names.

## How it works

- **Raw frame injection** — Overrides the driver's `ieee80211_raw_frame_sanity_check()` to allow management frames through `esp_wifi_80211_tx()`, which normally blocks deauth/disassoc frames.
- **Dual-mode WiFi** (`WIFI_AP_STA`) — Runs a control-panel access point (`ESP32-Deauth-Control`) and a station interface simultaneously, so it can serve the UI while transmitting attack frames.
- **Single-file web UI** — A polished dark-themed responsive control panel is served inline from the firmware (no filesystem needed), polling `/status` once per second to keep the packet counter and state indicators live.

## HTTP API

| Endpoint | Method | Purpose |
|---|---|---|
| `/` | GET | Web control panel |
| `/scan` | GET | Scan networks → JSON |
| `/attack?bssid=&channel=` | GET | Start deauth on target |
| `/stop` | GET | Stop deauth |
| `/beacon` | POST | Start beacon spam (body = SSID list) |
| `/beacon/stop` | GET | Stop beacon spam |
| `/status` | GET | Current state → JSON |

## Getting started

1. Flash the firmware with PlatformIO (`pio run -t upload`).
2. Connect to the WiFi network **`ESP32-Deauth-Control`** (password `calebRox123`).
3. Open **http://192.168.4.1** in a browser.
4. Scan, select a target, and start testing.

## Hardware

Any ESP32 dev board (`board = esp32dev`). Built on the Espressif32 platform with the Arduino framework.
