# Irrigation Controller – Agent Instructions

ESP32-based automated irrigation controller. Arduino framework, built with PlatformIO.

## Build & Upload

```sh
# Build
pio run

# Upload (OTA, requires device on network) — also ships the web UI, which is embedded
pio run --target upload        # uploads to hostname "Irrigation" via mDNS/OTA

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
| `web/` | Single-page web UI (`index.html`, `app.js`, `app.css`, `favicon.svg`), embedded into the firmware at build time |
| `scripts/embed_web.py` | PlatformIO pre-build script: minifies + gzips `web/` into the generated `src/web_assets.h` (git-ignored) |

Files created on the device at runtime (**not in the repo**):

| SPIFFS path | Role |
|-------------|------|
| `/wifi.txt` | SSID/password pairs (alternating lines: ssid, password, ssid, password…); written via the LCD Settings menu |
| `/mqtt.txt` | MQTT broker, port, username, password (one per line); written via the LCD Settings menu |
| `/web.txt` | Optional web login: username, password (one per line); written via LCD Settings > Web Login. Missing = no login |
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
Uses ESPAsyncWebServer. The UI in `web/` is static (no server-side templating) and is served pre-gzipped from flash with ETag caching; edit `web/` and run a normal build/upload. Data comes from a JSON API under `/api/*` (mutations are `POST` with an `application/json` body), and live updates arrive via `AsyncEventSource` at `/events` (`status` = JSON `{scheduled, remaining}`, `channels` = `"01000000"`). An optional digest login (`/web.txt`) covers every route. Channel display names are `channelDisplayNames` in `settings.h`. See `doc/WebModernization.md`.

### SPIFFS Files
SPIFFS only holds the device-created files above; nothing on it is served over HTTP. **Do not run `pio run --target uploadfs`**: flashing a SPIFFS image wipes `/schedules.json`, `/wifi.txt`, `/mqtt.txt` and `/web.txt`. If you ever must, export schedules from the web UI first and expect to re-enter WiFi/MQTT settings on the LCD — without WiFi the device can no longer be reached over OTA.

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
