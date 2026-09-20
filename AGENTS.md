# Irrigation Controller – Agent Instructions

ESP32-based automated irrigation controller. Arduino framework, built with PlatformIO.

## Build & Upload

```sh
# Build
pio run

# Upload (OTA, requires device on network)
pio run --target upload        # uploads to hostname "Irrigation" via mDNS/OTA

# Upload filesystem (SPIFFS data/) — flashes the WHOLE image, see "SPIFFS Files"
pio run --target uploadfs

# Serial monitor
pio device monitor             # 115200 baud
```

OTA upload requires the device to be connected to WiFi. Use `pio run --target upload --upload-port <IP>` if mDNS hostname resolution fails.

## Project Structure

| Path | Role |
|------|------|
| `src/main.cpp` | WiFi/MQTT/OTA init, web server routes, irrigation logic, main loop |
| `src/settings.h` | **Pin assignments**, I2C addresses, timing constants — change hardware config here |
| `src/main.h` | Global state variables, `MqttConfig` struct, web handler declarations |
| `src/globals.h` | `extern` declarations linking all subsystems |
| `src/Irrigation.h` / `irrigation.cpp` | `IrrigationSchedule` and `IrrigationSchedules` classes; JSON serialization |
| `src/display.h` / `display.cpp` | LCD 20×4 controller, scrolling, PWM dimming |
| `src/menu.h` / `menudef.h` / `menu.cpp` | LcdMenu wrapper, dynamic submenu generation from loaded schedules |
| `data/` | SPIFFS filesystem files served over HTTP |
| `data/Index`, `data/Schedule`, `data/ScheduleList` | HTML pages (**no file extension**), served by explicit routes with `%VARIABLE%` templating |
| `data/script.js`, `data/site.css` | Web UI script (opens `EventSource('/events')`) and styles |
| `data/channelNames.json` | Display labels for the 8 irrigation channels (UTF-8, Hungarian) |

Files created on the device at runtime (**not in the repo**, not in `data/`):

| SPIFFS path | Role |
|-------------|------|
| `/wifi.txt` | SSID/password pairs (alternating lines: ssid, password, ssid, password…); written via the LCD Settings menu |
| `/mqtt.txt` | MQTT broker, port, username, password (one per line); written via the LCD Settings menu |
| `/schedules.json` | Persistent schedule storage (loaded/saved via `convertFromJson`/`convertToJson`) |

## Key Conventions

### Relay Logic
Relays are **active-LOW**: `HIGH` = valve OFF, `LOW` = valve ON. Relays are initialized to `HIGH` at boot. Use `startChannel()` / `stopChannel()` / `toggleChannel()` from new code rather than writing GPIO directly. The timer-driven `ManageScheduledChannels()` / `ManageManualChannels()` do write pins directly (with their own MQTT/LCD updates), so keep them consistent if you change either.

### WiFi Credentials
`/wifi.txt` (SPIFFS) stores multiple SSID/password pairs as alternating lines. `readWiFiCredentials()` loads all pairs into `std::vector<WifiCredential> wifiCredentials`; `InitializeWiFi()` tries each in order. `saveWiFiCredentials(ssid, pwd)` updates an existing entry by SSID or appends a new one.

### Schedule Time Storage
Start times are stored as **minutes from midnight** (0–1439). Use `getStartTimeHours()` / `getStartTimeMinutes()` for display. Channel durations are 1-minute increments (0–90 min), scaled by a weight parameter (0–200%).

### MQTT Topics
Base topic prefix: `irrigation/` (hardcoded in `settings.h`). Command topic: `irrigation/command/#`. Status topics follow `irrigation/status/<key>`. Use `sendMQTTMessage(path, payload)` — path is appended to the base topic.

### Web Server
Uses ESPAsyncWebServer. Real-time status is pushed via `AsyncEventSource` at `/events`. Template placeholders in HTML files (e.g., `%MODE%`, `%SET_ACTIVE%`) are resolved in lambda processors passed to `request->send(SPIFFS, ...)`.

### SPIFFS Files
All files in `data/` are uploaded to SPIFFS. HTML files in `data/` use `%VARIABLE%`-style template substitution. Always call `pio run --target uploadfs` after changing files in `data/`.

**`uploadfs` flashes the whole SPIFFS image built from `data/`, so it wipes `/schedules.json`, `/wifi.txt` and `/mqtt.txt` created on the device.** Download schedules first (`/DownloadSchedules`; restore via `/UploadSchedules`) and expect to re-enter WiFi/MQTT settings on the LCD — without WiFi the device can no longer be reached over OTA.

### Menu System
Menu screens are dynamically rebuilt when schedules change (`menu->GenerateIrrigationSubmenu()`). `isMenuActive` gates the display loop — when true, rotary encoder input is processed; when false, the default status display runs.

## Hardware

- **Board:** ESP32 DoIt DevKit v1
- **Relay pins (GPIO):** 13, 32, 23, 25, 26, 27, 14, 12 (channels 1–8)
- **LCD:** 20×4 I2C at address `0x27`
- **RTC:** DS3231 at `0x68` (EEPROM at `0x57`)
- **Rotary encoder:** data pins 34, 35; button pin 15
- **LCD backlight PWM:** GPIO 5
- **Timezone:** CET-1CEST (Central European Time, hardcoded)

## Common Pitfalls

- **OTA hostname:** Upload target is `"Irrigation"` — requires mDNS or router hostname resolution. Use IP address if it fails.
- **ArduinoJson v7:** `JsonDocument` grows dynamically, so there is no capacity to size; just watch free heap if the schedule JSON gets large.
- **Schedule save:** Changes to schedules are not auto-saved; call `SaveSchedules(schedules)` after mutations. It already calls `menu->GenerateIrrigationSubmenu()`, so no separate call is needed.
- **Active-LOW relays:** Reading `LOW` on a relay pin means the channel is **ON**.
- **Manual end timer:** `irrigationManualEnd` (and `irrigationScheduledEnd`) are absolute `millis()` deadlines, with `0` meaning "none". `millis()` wraps after ~49 days, and the plain comparisons used (`millis() > end`, `millis() <= end`) are **not** wrap-safe; a fix would compare `(long)(millis() - end)` instead.
