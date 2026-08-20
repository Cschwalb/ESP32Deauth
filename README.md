ESP32 WiFi Test Tool
An ESP32-based 802.11 deauthentication test harness with a web control panel, intended for authorized security testing on networks you own or have written permission to assess.

What it does
The firmware turns an ESP32 into a self-contained WiFi testing appliance. On boot it starts its own WiFi access point (ESP32-Deauth-Control) and serves a web UI on port 80 — no external network, app, or serial console needed. You connect a phone or laptop to the ESP32's AP, open its IP in a browser, and drive everything from there.

How it works
Access Point + control panel — Runs in WIFI_AP_STA mode: the AP interface hosts the management UI while the station interface transmits test frames. A lightweight WebServer exposes a small REST-style API (/scan, /attack, /stop, /status).
Network discovery — /scan performs a WiFi scan and returns nearby networks as JSON (SSID, BSSID, channel, RSSI). The UI sorts them by signal strength and renders each with a signal-bar indicator.
Deauthentication frames — Builds a raw 802.11 deauth management frame (26 bytes, reason code 7) and transmits it via esp_wifi_80211_tx(). Selecting a target locks the ESP32 to that network's channel and repeatedly broadcasts frames spoofing the target AP's BSSID.
Driver filter bypass — Overrides ieee80211_raw_frame_sanity_check() so the ESP-IDF WiFi driver permits management-frame injection (paired with the --allow-multiple-definition linker flag).
Live telemetry — A cumulative packet counter and attack state are polled once per second via /status, so the UI shows a live "Deauthing" indicator and running frame count.
Interface
A dark-themed, mobile-friendly single-page UI: scan button with optional auto-refresh, a sorted network list with signal bars and channel chips, and a sticky status dock showing target, live packet count, and start/stop controls.

Intended use & caveat
This is a defensive/educational tool for testing the resilience of your own wireless networks to deauthentication attacks. Transmitting deauth frames against networks you don't own or aren't authorized to test is illegal in most jurisdictions. The UI carries an explicit authorization warning for that reason.

Want this as a README.md in the project root, or trimmed to a shorter one-paragraph blurb?
