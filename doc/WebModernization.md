# Web UI Modernization Plan

## Context

The web UI (`data/Index`, `data/Schedule`, `data/ScheduleList`, `script.js`, `site.css`) dates from 2022 and has several problems for an ESP32:

- **Slow and heavy.** Each page is server-templated with `%PLACEHOLDER%`. ESPAsyncWebServer cannot gzip a templated response (`WebResponses.cpp:787`: "Unable to process gzipped templates"). Every byte goes uncompressed, and the template processor runs on each request. `favicon.ico` is **100 KB**, about 85% of all web bytes. Nothing is cached.
- **Clunky behavior.** Every button does `fetch()` and then `location.reload()`. The layout is fixed at 600 px with 100 px side margins, which is bad on phones. Channel names arrive in a second request after the page has rendered. While a channel runs, the server pushes a "Scheduled (mm:ss left)" SSE text **every second**.
- **Duplicated code.** The "mode + time left" string is copy-pasted three times (`events.onConnect` `main.cpp:128`, `handle_OnRoot` `:636`, `sendStatusEvent` `:719`).
- **Security.** `serveStatic("/", SPIFFS, "/")` (`main.cpp:156`) serves `/wifi.txt` and `/mqtt.txt` in plaintext. Every state change is a plain GET (`/ToggleSwitch`, `/SetScheduled`, `/DeleteSchedule`, `/SetDimming`), so any page open in a browser on the LAN can trigger them (CSRF). `/UploadSchedules` overwrites `schedules.json` without validating it.
- **Deployment risk.** Every UI change needs `uploadfs`, which wipes `wifi.txt`, `mqtt.txt` and `schedules.json`. If WiFi is lost, the device can no longer be reached over OTA.

**Goal:** a fast, mobile-friendly single-page app with no reloads, embedded gzipped in the firmware and shipped with a normal OTA. The server becomes a small JSON API plus SSE. Security essentials are included.

**Decisions taken:** assets embedded in firmware · vanilla JS with no Node (the build step runs on PlatformIO's bundled Python) · basic security hardening included.

## Flash space check (answer: yes, plenty)

| | Bytes |
|---|---|
| OTA app slot (`app0`/`app1`, decoded from `partitions.bin`) | 1 310 720 |
| Current `firmware.bin` | 1 026 192 (78 %) |
| Free | ~284 KB |
| New UI, gzipped (estimate: HTML+JS+CSS ≈ 12–18 KB, SVG favicon < 1 KB) | ~15–20 KB (~1.5 % of slot) |

The JSON API code adds a few KB at most. Removing templating and `serveStatic` saves a little. The current data is about 18 KB of text plus the 100 KB icon, so the gzipped UI will still be smaller than today's favicon alone. No partition change is needed. **Rule:** keep the embedded web bundle under 40 KB gzipped. The build script prints its size on every build.

## ESP32 web best practices applied

1. **Serve pre-gzipped assets** with `Content-Encoding: gzip`. Compress at build time, never on the device.
2. **Use static assets plus a JSON API**, not server-side templating. Templating blocks gzip and caching and costs heap on each request.
3. **Cache with ETag**: hash each asset at build time. Answer `If-None-Match` with `304`, so a repeat visit transfers about 0 bytes. Send `Cache-Control: no-cache` so the browser revalidates and an OTA update shows up right away.
4. **Use few, small requests**: one HTML, one JS, one CSS, one tiny SVG favicon. Channel names come inside the state JSON.
5. **Push changes, not ticks.** SSE sends `{remaining seconds}` only when the state changes, and the browser counts down locally. This removes one SSE write per second per client.
6. **Stream JSON** with ArduinoJson straight into `AsyncResponseStream`/`AsyncJsonResponse`. Avoid big `String` concatenation, which fragments the heap.
7. **Make handlers non-blocking.** They only validate, call existing functions and reply.
8. **Use POST + JSON for mutations.** A cross-origin page cannot send `application/json` without a CORS preflight, which we never grant. This is a cheap, effective CSRF defence.

## Design

### 1. JSON API (`src/main.cpp`, web handler section)

| Method & path | Purpose | Reuses |
|---|---|---|
| `GET /api/state` | `{scheduled, remaining, channels:[0/1×8], names:[…], durationMin, durationMax, durationDef}` | new `buildStateJson()` |
| `POST /api/channel` `{ch, duration}` | toggle (ch 0 = all off) | `toggleChannel()` |
| `POST /api/mode` `{scheduled:bool}` | **set**, not toggle, so it is idempotent | same logic as `handle_OnSetScheduled` (`stopChannel(-1)`, MQTT, SSE) |
| `GET /api/schedules` | list with `id, start, daysToRun, weight, durations[], next4:"!-!-"` | `convertToJson()` logic in `irrigation.cpp`, `isValidForDay()` |
| `POST /api/schedules` `{id?, start, daysToRun, weight, durations[]}` | create or update | validation from `handle_OnSetSchedule`, `SaveSchedules()` |
| `POST /api/schedules/delete` `{id}` | delete | `removeSchedule()`, `SaveSchedules()` |
| `POST /api/dimming` `{value}` | backlight | `screen->DisplayDimm()` |
| `GET /api/schedules/export`, `POST /api/schedules/import` | keep download/upload | existing handlers; import parses with `convertFromJson` into a temp object and writes the file **only if it is valid** |

- Use `AsyncCallbackJsonWebHandler` (`AsyncJson.h`, ESPAsyncWebServer 3.11.0, already installed) for JSON bodies.
- **Consolidate status:** add one `unsigned long remainingSeconds()` and one `buildStateJson(JsonDocument&)`. Use them in `events.onConnect`, `sendStatusEvent` and `/api/state`. This deletes the three copy-pasted blocks. Update CLAUDE.md, which mentions them.
- **SSE events**: `channels` stays as it is (`"01000000"`). `status` becomes JSON `{"scheduled":true,"remaining":754}`. `onConnect` sends **both** events. Today it sends only status; the template's `%SET_ACTIVE%` covered channels.
- `SendIrrigationStatusUpdate()` keeps the 1 s MQTT `status/timeLeft` (out of scope) but stops the per-second `sendStatusEvent()`. SSE is sent only from `startChannel`/`stopChannel`/`ManageScheduledChannels`/`ManageManualChannels`/mode changes, all of which already call `sendStatusEvent()`/`sendChannelEvent()`. Also send it when a scheduled channel advances to the next one, because `remaining` changes then.

### 2. Embedded assets build step

- Move the UI source to a new `web/` folder: `index.html`, `app.js`, `app.css`, `favicon.svg`, `channelNames.json`. It sits outside `data/`, so `uploadfs` never touches it.
- New `scripts/embed_web.py`, registered as `extra_scripts = pre:scripts/embed_web.py` in `platformio.ini`. For each file in `web/`:
  - light minify (strip comments and leading whitespace; plain Python, no dependencies)
  - `gzip.compress(level=9, mtime=0)` (`mtime=0` keeps builds reproducible)
  - short ETag from SHA-1
  - write the generated header `src/web_assets.h`: `const uint8_t ..._gz[] PROGMEM`, plus a table `{path, mime, data, len, etag}`
  - regenerate only when a source is newer than the header; print the total gzipped size and fail the build if it exceeds 40 KB.
- Add `src/web_assets.h` to `.gitignore`, since it is generated.
- One generic route function in `InitializeWebServer()` walks the table. It answers `If-None-Match` with `304`, otherwise uses `beginResponse(200, mime, data, len)` + `Content-Encoding: gzip` + `ETag` + `Cache-Control: no-cache`. `/` maps to `index.html`.
- Channel names are now part of the firmware. `buildStateJson()` sends them from the embedded `channelNames.json`, parsed once at boot. Editing the names means editing `web/channelNames.json` and doing an OTA.

### 3. Frontend (vanilla, single page)

- **Hash routing** in `app.js`: `#/` (control), `#/schedules`, `#/schedule/<id|new>`. No page reloads.
- **Control view:** a mode badge with a live local countdown, a duration picker (min/max/default taken from `/api/state`), and 8 channel rows with large touch toggles (`<button aria-pressed>`). There are "All off" and a Scheduled/Manual switch. Updates are optimistic and then confirmed by SSE. If the request fails, the change is rolled back and a toast appears.
- **Schedules view:** a list of cards (start time, days, weight, next-4-days indicator) with edit/delete. Delete asks for confirmation in a `<dialog>`. Export and import buttons are here.
- **Schedule editor:** hour/minute selects (15 min steps, matching the LCD menu), days, weight, and 8 duration inputs (0–90, step 5). Client-side validation mirrors the server's.
- **Connection state:** `EventSource` reconnects automatically. A banner shows "Offline – reconnecting…", and `/api/state` is fetched again after a reconnect.
- **CSS:** mobile-first, CSS variables, `prefers-color-scheme` dark mode, system font stack (no web fonts), and `max-width: 40rem` instead of a fixed 600 px. Keep the green theme. Touch targets are at least 44 px.
- No external CDN. The device must work with no internet access.
- Keep the UI language English, as today. Channel names stay Hungarian (UTF-8).

### 4. Security essentials

- **Remove `serveStatic`** completely. Only the embedded assets and `/api/*` are served, so `wifi.txt`/`mqtt.txt` are no longer reachable.
- **Mutations are POST + `application/json` only.** The old GET routes are removed in the cleanup phase.
- **Optional login:** `AsyncAuthenticationMiddleware` (digest) applied to the server. Credentials come from a new optional `/web.txt` (user, password) entered via the LCD Settings menu, reusing the pattern of `commandMqttCallback`/`saveMqttCredentials`. If the file is missing, auth is off, which matches today's behavior. `/events` is covered too.
- **Bound inputs on the server**: channel 0–8, duration within `manualIrrigationDurationMin/Max`, start time 0–1439 (today it allows 1440, an off-by-one), `daysToRun`/weight within the allowed sets.
- Out of scope and noted only: OTA password (`main.cpp:177`) and HTTPS (too heavy for this device).

## Implementation phases (each one builds and works on its own)

1. **API + status consolidation:** add `remainingSeconds()`, `buildStateJson()` and the `/api/*` routes. Switch SSE `status` to JSON. The old pages keep working; adjust `script.js` to parse the new status.
2. **Embed pipeline:** `scripts/embed_web.py`, `platformio.ini` entry, generated header, generic asset route. Ship the *current* pages through it to prove it works.
3. **New SPA:** write `web/index.html`, `app.js`, `app.css`, `favicon.svg`.
4. **Security:** remove `serveStatic` and the old GET routes, add the import validation, add optional digest auth with the LCD `/web.txt` entry.
5. **Cleanup and docs:** delete `data/Index`, `data/Schedule`, `data/ScheduleList`, `script.js`, `site.css`, `favicon.ico`, `channelNames.json` and the handlers `handle_OnRoot`, `handle_OnGetSchedule`, `handle_onScheduleList`, `handle_OnToggleSwitch`, `handle_OnSetScheduled`, `handle_OnDeleteSchedule`, `handle_OnSetDimming`, `handle_OnGetSettings`. Update `CLAUDE.md` and `AGENTS.md`: the web UI lives in `web/` and is embedded, `uploadfs` is no longer needed for the UI (and should be avoided), and the status helpers are now in one place.

## Critical files

- `src/main.cpp`: `InitializeWebServer()` (:125), handlers (:433–750), `sendStatusEvent`/`sendChannelEvent`, `SendIrrigationStatusUpdate` (:945)
- `src/main.h`: handler declarations, globals
- `src/irrigation.cpp` / `Irrigation.h`: reuse `convertToJson`/`convertFromJson` and the schedule getters
- `src/menu.cpp`, `menudef.h`: Settings entry for web credentials (phase 4)
- `platformio.ini`: `extra_scripts`
- new: `scripts/embed_web.py`, `web/*`, generated `src/web_assets.h`
- `.gitignore`, `CLAUDE.md`, `AGENTS.md`

## Verification

There are no automated tests, so verification is by build and by hand on the device:

1. `pio run`: the build passes and the script prints the gzipped bundle size (expect < 25 KB). Check `firmware.bin` stays below 1 310 720 bytes (expect about 1.04 MB).
2. `pio run --target upload` (OTA). Confirm that `wifi.txt`, `mqtt.txt` and `schedules.json` survive, since no `uploadfs` is needed.
3. In the browser DevTools Network tab: the first load sends `Content-Encoding: gzip`, the total is under 25 KB, and there are 4 requests. A reload returns `304` for every asset.
4. `curl http://Irrigation/wifi.txt` returns 404. `curl http://Irrigation/ToggleSwitch?ch=1` returns 404. `curl -X POST -H 'Content-Type: application/json' -d '{"ch":1,"duration":1}' http://Irrigation/api/channel` toggles the relay.
5. Functional checks on a phone and a desktop:
   - toggle a channel: the relay clicks, the UI updates without a reload, and the LCD and MQTT `status/...` update
   - the countdown ticks locally
   - a second browser window updates through SSE
   - switching scheduled/manual works
   - create, edit and delete a schedule; the LCD schedule menu reflects the change (`SaveSchedules` → `GenerateIrrigationSubmenu`)
   - export and then import a schedule file
   - importing an invalid JSON file is rejected and the existing schedules are kept
6. Stop WiFi or reboot the device: the "Offline" banner appears and the page recovers by itself.
7. With `/web.txt` set, the browser asks for login, and `/events` and `/api/*` return 401 without credentials.
8. Watch heap in `pio device monitor` (log `ESP.getFreeHeap()` hourly in `handleTimer`, temporarily) with 2–3 open tabs. There should be no steady decline.

## Implementation notes (branch `feature/web-modernization`)

All five phases are implemented, one commit each. Differences from the plan above:

- **API paths** avoid prefix overlaps in the router: `POST /api/schedule/save`, `POST /api/schedule/delete`, `GET /api/export`, `POST /api/import`. Schedule bodies use the `schedules.json` field names (`startTime`, `daysToRun`, `weight`, `channelDurations`), so save and import share one validator (`scheduleFromJson()`).
- **Channel names** are a C array (`channelDisplayNames` in `src/settings.h`), not an embedded JSON file parsed at boot. It is simpler and needs no parsing.
- **Editor durations** use 1-minute steps, matching the LCD menu, instead of 5. With 5-minute steps the browser would block saving a schedule that has values set on the LCD.
- **Phase 2** could not ship the old pages through the pipeline, because they depend on server-side templating. The pipeline was proven with `favicon.svg`, and the SPA followed in phase 3.
- **The JS minifier** only removes whole-line `//` comments and indentation. Removing `/* */` broke code when a line comment contained `/*`.
- **Mode switching** goes through `setScheduledMode()` for web, MQTT and LCD. A real switch stops all channels, which also fixes a valve staying open after switching from scheduled to manual.
- **Result:** the web bundle is about 7.3 KB gzipped and flash use is 78.9 % (1 033 685 / 1 310 720 bytes).
- **Not changed (out of scope):** MQTT run times are still unbounded, the manual-expiry check in `ManageManualChannels()` is still not wrap-safe, and OTA still has no password.
