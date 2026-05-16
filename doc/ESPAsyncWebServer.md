# ESPAsyncWebServer Migration Documentation

## Source

- **Library**: `ESP32Async/ESPAsyncWebServer`  
- **Documentation**: <https://ESP32Async.github.io/ESPAsyncWebServer/>  
- **Repository**: <https://github.com/ESP32Async/ESPAsyncWebServer>  
- **PlatformIO Registry**: <https://registry.platformio.org/libraries/ESP32Async/ESPAsyncWebServer>

> **Note**: The original library at `me-no-dev/ESPAsyncWebServer` has been archived (Jan 20, 2025).  
> The active fork is maintained at `ESP32Async/ESPAsyncWebServer`.

---

## PlatformIO Dependencies (ESP32)

```ini
lib_deps =
    ESP32Async/ESPAsyncWebServer
    ESP32Async/AsyncTCP
```

---

## Key Differences vs `WebServer`

| Feature | WebServer | ESPAsyncWebServer |
|---|---|---|
| Include | `<WebServer.h>` | `<ESPAsyncWebServer.h>` + `<AsyncTCP.h>` |
| Server type | `WebServer server(80)` | `AsyncWebServer server(80)` |
| Handler signature | `void handler()` | `void handler(AsyncWebServerRequest *request)` |
| Polling loop | `server.handleClient()` required | Not needed (async) |
| Route method arg | Optional | Required |
| Send response | `server.send(code, type, body)` | `request->send(code, type, body)` |
| Get param | `server.arg("name")` | `request->arg("name")` (compat) or `request->getParam("name")->value()` |
| Has param | `server.hasArg("name")` | `request->hasArg("name")` (compat) or `request->hasParam("name")` |
| POST param | `server.arg("name")` | `request->getParam("name", true)->value()` (isPost=true) |
| Redirect | `server.sendHeader("Location", url, true); server.send(303, ...)` | `request->redirect(url)` |
| Static files | `server.serveStatic("/", SPIFFS, "/")` | `server.serveStatic("/", SPIFFS, "/")` (same) |
| Not found | `server.onNotFound(handler)` | `server.onNotFound(handler)` (same, new signature) |

---

## Server Setup

```cpp
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

void setup() {
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.serveStatic("/", SPIFFS, "/");

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
}

void loop() {
    // No server.handleClient() needed!
}
```

---

## Request Handling

### GET Parameters

```cpp
server.on("/example", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if param exists
    if (request->hasParam("id")) {
        String id = request->getParam("id")->value();
        // or compatibility style:
        String id2 = request->arg("id");
    }
    request->send(200, "text/plain", "OK");
});
```

### POST Parameters (URL-encoded form)

```cpp
server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request) {
    // isPost = true (second argument)
    if (request->hasParam("name", true)) {
        String value = request->getParam("name", true)->value();
    }
    // Compatibility style also works:
    String value2 = request->arg("name");
    request->send(200, "text/plain", "Saved");
});
```

---

## Responses

```cpp
// Simple string response
request->send(200, "text/plain", "Hello World");

// JSON response
request->send(200, "application/json", "{\"status\":\"OK\"}");

// HTML error
request->send(400, "text/html", "Bad Request");

// Redirect
request->redirect("/other-page");

// Send file from SPIFFS
request->send(SPIFFS, "/index.htm", "text/html");

// Not found
request->send(404, "text/plain", "Not found");
```

---

## Handler Function Pattern

```cpp
// Declaration (e.g., in header)
void handle_OnSomething(AsyncWebServerRequest *request);

// Registration
server.on("/something", HTTP_GET, handle_OnSomething);

// Implementation
void handle_OnSomething(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
}
```

---

## Migration Notes for This Project

### Changes in `main.cpp`

1. Replaced `#include <WebServer.h>` with `#include <AsyncTCP.h>` and `#include <ESPAsyncWebServer.h>`
2. Changed `WebServer server(80)` → `AsyncWebServer server(80)`
3. Updated `InitializeWebServer()` — all routes now specify HTTP method explicitly
4. All handler functions updated to accept `AsyncWebServerRequest *request`
5. All `server.hasArg()`/`server.arg()`/`server.send()` calls converted to `request->` equivalents
6. `server.sendHeader("Location", ...) + server.send(303, ...)` replaced with `request->redirect(...)`
7. Removed `server.handleClient()` from `loop()` — ESPAsyncWebServer is non-blocking

### Changes in `main.h`

- Forward declarations updated to include `AsyncWebServerRequest *request` parameter
- Added `#include <ESPAsyncWebServer.h>` guard

### Changes in `platformio.ini`

- Added `ESP32Async/ESPAsyncWebServer` and `ESP32Async/AsyncTCP` to `lib_deps`
