# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

ESP32 (DoIt DevKit v1) 8-channel irrigation controller. Arduino framework, PlatformIO, single env `esp32doit-devkit-v1`. There are no tests and no linter — `test/` is an empty PlatformIO stub. `AGENTS.md` has a hardware/pin summary; pins and I2C addresses live in `src/settings.h`.

## Commands

```sh
pio run                          # build
pio run --target upload          # OTA upload to host "Irrigation" (upload_protocol = espota)
pio run --target upload --upload-port <IP>   # if mDNS name doesn't resolve
pio run --target uploadfs        # upload data/ to SPIFFS (required after ANY change in data/)
pio device monitor               # serial, 115200
```

Upload is OTA-only as configured: the device must already be on WiFi and running firmware with OTA enabled. A first flash over USB needs `upload_protocol` changed (or overridden) from `espota` plus `--upload-port COMx`.

## Architecture

Almost everything is in `src/main.cpp` (~1400 lines, in sections: inits → file/credentials → HTTP handlers → MQTT → irrigation logic → `setup`/`loop` → menu callbacks). `main.h` **defines** globals (not just declares them) and is included only by `main.cpp` — it is effectively part of that translation unit, so don't include it elsewhere; other files see state through `globals.h` `extern`s.

**Main loop** (`loop()`): MQTT + ArduinoOTA (only if WiFi up) → `ManageIrrigation()` → either the menu (`isMenuActive`, rotary via LcdMenu) or the status display + `handleRotary()` → `handleTimer()` (hourly: WiFi reconnect, NTP→RTC sync, timezone). Everything is cooperative polling; nothing may block for long.

**Two mutually exclusive irrigation modes**, switched by `irrigationScheduleEnabled`:
- *Scheduled*: `ManageScheduledChannels()` polls every `irrigationCheckInterval` (10 s), reads the DS3231 (`rtc.now()`), and for each schedule checks both yesterday and today (schedules can cross midnight). A schedule runs its 8 channels **sequentially** starting at `startTime`; channel *j*'s duration is `duration[j] * weight / 100` minutes. Only one channel runs at a time; the function reconciles all relay pins to match the computed channel, and sets `irrigationScheduledEnd` (a `millis()` deadline) for the time-left display. It also zeroes `irrigationManualEnd`.
- *Manual*: `startChannel()` **forces manual mode** (sets `irrigationScheduleEnabled = false`), turns off other channels, and sets `irrigationManualEnd`. `ManageManualChannels()` shuts everything off when it expires. Entry points: LCD menu, web `/ToggleSwitch`, MQTT `command/channelN`. Re-enabling schedule mode is via web `/SetScheduled` (a toggle), MQTT `command/scheduled`, or the menu.

**State fan-out**: every state change must be pushed to three consumers — the LCD (`screen->DisplayMessage`, `displayOutChange`), MQTT (`sendMQTTMessage("status/...")`), and web clients via SSE (`sendChannelEvent()` → event `channels`, a string like `"01000000"`; `sendStatusEvent()` → event `status`, the "Scheduled (mm:ss left)" text). When adding a code path that changes relays or mode, replicate what `startChannel`/`stopChannel` do. The "mode + time left" string is built by copy-paste in three places (`events.onConnect`, `handle_OnRoot`, `sendStatusEvent`) — update all if you change the format.

**Persistence** (SPIFFS, no external DB):
- `/schedules.json` is written by `SaveSchedules()` (which already calls `menu->GenerateIrrigationSubmenu()`); (de)serialized by `convertToJson`/`convertFromJson` in `irrigation.cpp`. `IrrigationSchedules` holds a `std::map<int, IrrigationSchedule>` keyed by index; `removeSchedule` re-indexes. The file is **not** in the repo — if missing, boot logs "Failed to open schedules" and there are no schedules.
- `/wifi.txt` and `/mqtt.txt` are **not** in the repo either; they're created on-device through the LCD Settings menu (`commandWifiCallback`/`commandMqttCallback` in `menu.cpp`) and read at boot.
- **`uploadfs` flashes the whole SPIFFS image built from `data/`, so it wipes `schedules.json`, `wifi.txt` and `mqtt.txt`** created on the device. Download schedules first (`/DownloadSchedules`; restore via `/UploadSchedules`) and expect to re-enter WiFi/MQTT settings on the LCD — if WiFi is lost, the device can no longer be reached over OTA.

**Web UI**: `data/` files `Index`, `Schedule`, `ScheduleList` are HTML **without extensions** (served by explicit routes with `%PLACEHOLDER%` templating, e.g. `/Index` in `handle_OnRoot`); `script.js`/`site.css`/`channelNames.json` are served by `serveStatic("/", SPIFFS, "/")`. `script.js` opens an `EventSource('/events')` and listens for `status` and `channels`. Routes are registered in `InitializeWebServer()` at boot, but `server.begin()` only happens inside a successful `InitializeWiFi()`.

**Menu** (`menu.cpp`, `menudef.h`): uses LcdMenu v5 macros. Static screens (`mainScreen`, `settingsScreen`, wifi/mqtt) are defined in `menudef.h`; manual/schedule screens are `nullptr` globals rebuilt at runtime by `Menu::Generate*` methods. Reading widget values back in callbacks (e.g. `commandScheduleSaveCallback`, `toggleChannelCallback`) uses `static_cast`s keyed to **item index** in the screen — reordering items in a generated screen silently breaks those callbacks.

## Gotchas

- Relays are active-LOW (`LOW` = valve on). Prefer `startChannel`/`stopChannel`/`toggleChannel` from new code; the timer-driven `ManageScheduledChannels`/`ManageManualChannels` write pins directly (with their own MQTT/LCD updates), so keep them consistent if you touch either.
- `irrigationManualEnd`/`irrigationScheduledEnd` are `millis()` deadlines with `0` meaning "none". Manual expiry is checked as `millis() > end`, which is not wrap-safe across the ~49-day rollover despite the comment in `AGENTS.md`.
- `JsonDocument` is ArduinoJson v7 (dynamic sizing) — no capacity to tune, despite what `AGENTS.md` says about heap sizing.
- Start times are minutes from midnight (0–1439); `daysToRun` is an index into `daysToRunValues` (`"All"`, `"Odd"`, `"Even"`, `"Every3days"`…) and `isValidForDayRun` interprets it (values ≥2 use `epochDay % value`). Channel count (8) is the compile-time constant `irrigationChannelNumber` in `Irrigation.h`, independent of the relay pin list in `main.cpp`.
- MQTT topic prefix is `irrigation/` (`settings.h`); commands on `irrigation/command/{channelN|off|scheduled|status}`, payloads `ON`, `OFF`, `ON:<min>`, or a bare number of minutes. Note `command/off` requires payload `ON`.
- Timezone string `CET-1CEST,M3.5.0/2,M10.5.0/3` is hardcoded in `main.h`; user-visible display strings are a mix of English and Hungarian (`channelNames.json` is Hungarian, UTF-8).
- `doc/ESPAsyncWebServer.md` documents the migration from the sync `WebServer` to `ESP32Async/ESPAsyncWebServer` — consult it for handler/response API differences before writing web handlers.
