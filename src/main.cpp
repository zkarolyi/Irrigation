#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <main.h>
#include "SPIFFS.h"
#include <ArduinoOTA.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cmath>
#include "settings.h"
#include "globals.h"
#include "Irrigation.h"
#include "display.h"
#include "menu.h"
using namespace std;
#include <HTTPClient.h>

struct WifiCredential { String ssid; String password; };
std::vector<WifiCredential> wifiCredentials;

RTC_DS3231 rtc;

AsyncWebServer server(80);
AsyncEventSource events("/events");
IrrigationSchedules schedules;
Display *screen = nullptr;
Menu *menu = nullptr;
bool isMenuActive = false;
std::vector<int> relayPins = {RELAY_PIN_1, RELAY_PIN_2, RELAY_PIN_3, RELAY_PIN_4, RELAY_PIN_5, RELAY_PIN_6, RELAY_PIN_7, RELAY_PIN_8};

MqttConfig mqttConfig = {
    .topic = mqttTopic,
    .clientId = mqttClientId,
};
WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool mqttSubscribed = false;

// ##########################################
// ###      ###  ####  ###      ###        ##
// #####  #####    ##  #####  ########  #####
// #####  #####  #  #  #####  ########  #####
// #####  #####  ##    #####  ########  #####
// ###      ###  ####  ###      ######  #####
// ##########################################

// Inits
void InicializeRelays()
{
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    pinMode(schedules.getPin(i), OUTPUT);
    digitalWrite(schedules.getPin(i), HIGH);
  }
}

// https://arduinojson.org/
void InitializeSchedules()
{
  File file = SPIFFS.open(schedulesFile);
  if (!file)
  {
    screen->DisplayMessage("Failed to open schedules", true, true);
    return;
  }

  String json = file.readString();
  file.close();

  convertFromJson(json, schedules);
}

void InitializeChannelNames()
{
  File file = SPIFFS.open("/channelNames.json");
  if (!file)
    return;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error)
    return;
  channelNames.clear();
  for (JsonVariant name : doc.as<JsonArray>())
    channelNames.push_back(name.as<String>());
}

void UpdateTimeZone()
{
  setenv("TZ", tz, 1);
  tzset();
  Serial.printf("Időzóna beállítva: %s\n", tz);
}

bool InitializeWiFi()
{
  for (const auto &cred : wifiCredentials)
  {
    screen->DisplayMessage("Connecting to ");
    screen->DisplayMessage(cred.ssid, false, true);
    Serial.println("Connect to router");

    WiFi.disconnect();
    WiFi.hostname(HOSTNAME);
    WiFi.begin(cred.ssid.c_str(), cred.password.c_str());

    screen->DisplayMessage(".", true, false);
    int countDown = 20;
    while (WiFi.status() != WL_CONNECTED && countDown-- > 0)
    {
      delay(1000);
      screen->DisplayMessage(".", false, false);
    }
    if (WiFi.status() == WL_CONNECTED)
      break;
    screen->DisplayMessage("Failed, trying next", true, true);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    screen->DisplayMessage("WI-FI connected", true, true);
    screen->DisplayMessage("IP address: ");
    screen->DisplayMessage(WiFi.localIP().toString(), false, true);

    wifiIpAddress = WiFi.localIP().toString();
    wifiDnsIp = WiFi.dnsIP().toString();
    wifiGatewayIp = WiFi.gatewayIP().toString();
    wifiHostname = WiFi.getHostname();
    wifiMacAddress = WiFi.macAddress();
    wifiSsid = WiFi.SSID();
    configTime(0, 0, ntpServer);
    server.begin();
    return true;
  }
  screen->DisplayMessage("WI-FI connection failed", true, true);
  screen->DisplayMessage("HTTP server not started", true, true);
  return false;
}


void InitializeWebServer()
{
  server.on("/", HTTP_GET, handle_OnRoot);
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send(statusJson().c_str(), "status", millis(), 1000);
    client->send(channelStates().c_str(), "channels", millis());
  });
  server.addHandler(&events);

  // JSON API
  server.on("/api/state", HTTP_GET, handle_ApiState);
  server.on("/api/schedules", HTTP_GET, handle_ApiSchedules);
  server.on("/api/export", HTTP_GET, handle_ApiExport);
  addJsonPostHandler("/api/channel", handle_ApiChannel);
  addJsonPostHandler("/api/mode", handle_ApiMode);
  addJsonPostHandler("/api/schedule/save", handle_ApiScheduleSave);
  addJsonPostHandler("/api/schedule/delete", handle_ApiScheduleDelete);
  addJsonPostHandler("/api/dimming", handle_ApiDimming);
  addJsonPostHandler("/api/import", handle_ApiImport, 16384);

  server.on("/ToggleSwitch", HTTP_GET, handle_OnToggleSwitch);
  server.on("/SetScheduled", HTTP_GET, handle_OnSetScheduled);
  server.on("/Schedule", HTTP_GET, handle_OnGetSchedule);
  server.on("/Schedule", HTTP_POST, handle_OnSetSchedule);
  server.on("/SetDimming", HTTP_GET, handle_OnSetDimming);
  server.on("/ScheduleList", HTTP_GET, handle_onScheduleList);
  server.on("/GetSettings", HTTP_GET, handle_OnGetSettings);
  server.on("/DownloadSchedules", HTTP_GET, handle_OnDownloadSchedules);
  server.on("/DeleteSchedule", HTTP_GET, handle_OnDeleteSchedule);
  server.on("/UploadSchedules", HTTP_POST,
    [](AsyncWebServerRequest *request) { request->redirect("/ScheduleList"); },
    handle_UploadSchedules);
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(handle_NotFound);
  screen->DisplayMessage("HTTP server initialized", true, true);
}

void InitializeOTA()
{
  // Avoid multiple initializations of OTA
  static bool otaInitialized = false;
  if (otaInitialized)
    return;
  otaInitialized = true;

  // OTA
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  //
  // Hostname defaults to esp32-[MAC]
  ArduinoOTA.setHostname(HOSTNAME);
  //
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  //
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
      .onStart([]()
               {
                 String type;
                 if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                 else // U_SPIFFS
                   type = "filesystem";

                 screen->DisplayMessage("Updating " + type, true, true); })
      .onEnd([]()
             { screen->DisplayMessage("End", true, true); })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    // esp_task_wdt_reset();                   
                    screen->DisplayMessage("Progress: " + String((progress * 100) / total) + "%", true, true); })
      .onError([](ota_error_t error)
               {
                 screen->DisplayMessage("Error[" + String(error) + "]: ", true, true);
                 if (error == OTA_AUTH_ERROR) screen->DisplayMessage("Auth Failed", false, true);
                 else if (error == OTA_BEGIN_ERROR) screen->DisplayMessage("Begin Failed", false, true);
                 else if (error == OTA_CONNECT_ERROR) screen->DisplayMessage("Connect Failed", false, true);
                 else if (error == OTA_RECEIVE_ERROR) screen->DisplayMessage("Receive Failed", false, true);
                 else if (error == OTA_END_ERROR) screen->DisplayMessage("End Failed", false, true); });

  ArduinoOTA.begin();
}

void InitializeRTC()
{
  // RTC
  if (!rtc.begin())
  {
    screen->DisplayMessage("Couldn't find RTC", true, true);
    return;
  }
  rtc.disable32K();
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  if (!rtc.lostPower())
  {
    screen->DisplayMessage("RTC is powered", true, true);
  }
  else
  {
    screen->DisplayMessage("RTC lost power", true, true);
  }
}

void InitializeMQTT()
{
  static bool mqttInitialized = false;
  if (mqttInitialized)
    return;

  if (!readMqttCredentials() || !mqttConfig.isValid())
  {
    Serial.println("MQTT config invalid, skipping InitializeMQTT");
    screen->DisplayMessage("MQTT config invalid", true, true);
    return;
  }

  mqttClient.setServer(mqttConfig.broker, mqttConfig.port);
  mqttClient.setCallback(mqttMessageHandler);
  mqttInitialized = true;
  Serial.println("MQTT initialized");
}

// ##########################################
// ##        ###      ###  ########        ##
// ##  ###########  #####  ########  ########
// ##      #######  #####  ########      ####
// ##  ###########  #####  ########  ########
// ##  #########      ###        ##        ##
// ##########################################

// File
void SaveSchedules(IrrigationSchedules &schedules)
{
  File file = SPIFFS.open(schedulesFile, "w");
  if (!file)
  {
    screen->DisplayMessage("Failed to open file for writing", true, true);
    return;
  }

  String json = convertToJson(schedules);
  if (file.write((uint8_t *)json.c_str(), json.length()) != json.length())
  {
    screen->DisplayMessage("Failed to write schedules", true, true);
  }

  file.close();
  menu->GenerateIrrigationSubmenu();
}

void InitFS()
{
  if (!SPIFFS.begin(true))
  {
    screen->DisplayMessage("An Error has occurred while mounting SPIFFS", true, true);
    return;
  }
}

void saveWiFiCredentials(const char *newSsid, const char *newPassword)
{
  Serial.printf("Saving wifi credentials: %s, %s\n", newSsid, newPassword);

  // Update existing entry or append a new one
  bool found = false;
  for (auto &cred : wifiCredentials)
  {
    if (cred.ssid == newSsid)
    {
      cred.password = newPassword;
      found = true;
      break;
    }
  }
  if (!found)
    wifiCredentials.push_back({newSsid, newPassword});

  File file = SPIFFS.open("/wifi.txt", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  for (const auto &cred : wifiCredentials)
  {
    file.println(cred.ssid);
    file.println(cred.password);
  }
  file.close();

  Serial.println("Wifi credentials saved");
  InitializeWiFi();
}

boolean readWiFiCredentials()
{
  if (!SPIFFS.exists("/wifi.txt"))
  {
    Serial.println("No wifi file found");
    return false;
  }
  File file = SPIFFS.open("/wifi.txt", FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return false;
  }

  wifiCredentials.clear();
  while (file.available())
  {
    String s = file.readStringUntil('\n');
    String p = file.readStringUntil('\n');
    s.trim();
    p.trim();
    if (s.length() > 0 && p.length() > 0)
      wifiCredentials.push_back({s, p});
  }
  file.close();

  Serial.printf("Wifi read from file: %d credential(s)\n", (int)wifiCredentials.size());
  return !wifiCredentials.empty();
}

void saveMqttCredentials(const char *broker, int port, const char *username, const char *password)
{
  mqttConfig.free();

  mqttConfig.broker = strdup(broker);
  mqttConfig.port = port;
  mqttConfig.username = strdup(username);
  mqttConfig.password = strdup(password);

  Serial.printf("Saving MQTT credentials: %s, %d, %s, %s\n",
                mqttConfig.broker,
                mqttConfig.port,
                mqttConfig.username,
                mqttConfig.password);

  File file = SPIFFS.open("/mqtt.txt", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.println(mqttConfig.broker);
  file.println(mqttConfig.port);
  file.println(mqttConfig.username);
  file.println(mqttConfig.password);
  file.close();

  Serial.println("MQTT credentials saved");
}

boolean readMqttCredentials()
{
  if (!SPIFFS.exists("/mqtt.txt"))
  {
    Serial.println("No mqtt file found");
    return false;
  }
  File file = SPIFFS.open("/mqtt.txt", FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return false;
  }
  String brokerString = file.readStringUntil('\n');
  String portString = file.readStringUntil('\n');
  String usernameString = file.readStringUntil('\n');
  String passwordString = file.readStringUntil('\n');
  file.close();

  if (!brokerString.length() || !portString.length() || !usernameString.length() || !passwordString.length())
  {
    Serial.println("MQTT file format incorrect");
    return false;
  }

  brokerString.trim();
  portString.trim();
  usernameString.trim();
  passwordString.trim();

  mqttConfig.free();

  mqttConfig.broker = strdup(brokerString.c_str());
  mqttConfig.port = portString.toInt();
  mqttConfig.username = strdup(usernameString.c_str());
  mqttConfig.password = strdup(passwordString.c_str());

  Serial.println("MQTT read from file");
  return true;
}

// ##################################################################################
// ##  ####  ###      ###  ####  ##       ###  ########        ##       ####      ###
// ##  ####  ##  ####  ##    ##  ##  ####  ##  ########  ########  ####  ##  ########
// ##        ##        ##  #  #  ##  ####  ##  ########      ####       ####      ###
// ##  ####  ##  ####  ##  ##    ##  ####  ##  ########  ########  ##  ##########  ##
// ##  ####  ##  ####  ##  ####  ##       ###        ##        ##  ####  ###      ###
// ##################################################################################

// Handlers
void handle_OnSetDimming(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set dimming", true, true);
  int dimm = request->hasArg("dimm") ? request->arg("dimm").toInt() : 255;
  if (dimm < 0 || dimm > 255)
  {
    screen->DisplayMessage("Invalid dimming", true, true);
    request->send(400, "text/html", "Invalid dimming");
    return;
  }
  screen->DisplayDimm(dimm);
  request->send(200, "text/html", "OK (" + String(dimm) + ")");
  screen->DisplayMessage("Finished on: " + String(dimm), true, true);
}

void handle_OnGetSchedule(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle get schedule", true, true);

  int id = request->hasArg("id") ? request->arg("id").toInt() : -1;

  int sth = 0, stm = 0, dtr = 0, weight = 100;
  std::vector<int> durations(irrigationChannelNumber, 0);

  if (id >= 0 && id < schedules.getNumberOfSchedules())
  {
    IrrigationSchedule sch = schedules.getSchedule(id);
    sth    = sch.getStartTimeHours();
    stm    = sch.getStartTimeMinutes();
    dtr    = (int)sch.getDaysToRun();
    screen->DisplayMessage("R:" + String(dtr), true, false);
    weight = sch.getWeight();
    for (int i = 0; i < irrigationChannelNumber; i++)
      durations[i] = sch.getChannelDuration(i);
  }
  else
  {
    id = -1;
  }

  request->send(SPIFFS, "/Schedule", "text/html", false,
    [id, sth, stm, dtr, weight, durations](const String &var) -> String {
      if (var == "ID")              return String(id);
      if (var.startsWith("STH"))   return sth == var.substring(3).toInt() ? " selected" : "";
      if (var.startsWith("STM"))   return stm == var.substring(3).toInt() ? " selected" : "";
      if (var.startsWith("DTR"))   return dtr == var.substring(3).toInt() ? " selected" : "";
      if (var.startsWith("W"))     return weight == var.substring(1).toInt() ? " selected" : "";
      if (var.startsWith("DURATION")) {
        int ch = var.substring(8).toInt() - 1;
        if (ch >= 0 && ch < (int)durations.size()) return String(durations[ch]);
      }
      return String();
    });

  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnSetSchedule(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set schedule", true, true);
  int id = request->hasArg("id") ? request->arg("id").toInt() : -1;
  int getStartTime = request->hasArg("startTimeHour") && request->hasArg("startTimeMinute") ? request->arg("startTimeHour").toInt() * 60 + request->arg("startTimeMinute").toInt() : -1;
  if (getStartTime < 0 || getStartTime > 1440)
  {
    screen->DisplayMessage("Invalid start time", true, true);
    request->send(400, "text/html", "Invalid start time");
    return;
  }

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    String argName = "duration" + String(i + 1);
    if (!request->hasArg(argName) || request->arg(argName).length() == 0 || request->arg(argName).toInt() < manualIrrigationDurationMin || request->arg(argName).toInt() > manualIrrigationDurationMax)
    {
      screen->DisplayMessage("Invalid duration", true, true);
      request->send(400, "text/html", "Invalid duration" + String(i + 1));
      return;
    }
  }

  IrrigationSchedule currSchedule;
  if (id >= 0 && id < schedules.getNumberOfSchedules())
  {
    currSchedule = schedules.getSchedule(id);
  }

  currSchedule.setStartTime(getStartTime / 60, getStartTime % 60);
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    currSchedule.addChannelDuration(i, request->arg("duration" + String(i + 1)).toInt());
  }
  currSchedule.setDaysToRun(request->arg("daysToRun").toInt());
  currSchedule.setWeight(request->arg("weight").toInt());

  if (id >= 0 && id < schedules.getNumberOfSchedules())
  {
    schedules.updateSchedule(id, currSchedule);
  }
  else
  {
    schedules.addSchedule(currSchedule);
  }
  SaveSchedules(schedules);
  request->redirect("/ScheduleList");
  screen->DisplayMessage("Finished.", true, true);
}

void handle_onScheduleList(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle schedule list", true, true);

  String lines;
  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule sch = schedules.getSchedule(i);
    lines += "<div class=\"listItem\">";
    lines += String(sch.getStartTimeString().c_str()) + " ";
    if (!rtc.lostPower())
    {
      for (int d = 0; d <= 3; ++d)
      {
        DateTime futureDay = rtc.now() + TimeSpan(d, 0, 0, 0);
        lines += sch.isValidForDay(futureDay) ? "!" : "-";
      }
    }
    lines += "</div>";
    lines += "<div class=\"listItem\">";
    lines += String(sch.getWeight());
    lines += "</div>";
    lines += "<div class=\"listItem\">";
    lines += "<a href=\"Schedule?id=" + String(i) + "\" class=\"linkButton\">Details</a>";
    lines += " <a href=\"DeleteSchedule?id=" + String(i) + "\" class=\"linkButton linkButton--danger\" onclick=\"return confirm('Delete this schedule?')\">Delete</a>";
    lines += "</div>\n";
  }

  request->send(SPIFFS, "/ScheduleList", "text/html", false,
    [lines](const String &var) -> String {
      if (var == "SCHEDULES") return lines;
      return String();
    });

  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnDeleteSchedule(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  int id = request->hasArg("id") ? request->arg("id").toInt() : -1;
  if (id < 0 || id >= schedules.getNumberOfSchedules())
  {
    request->send(400, "text/html", "Invalid schedule id");
    return;
  }
  schedules.removeSchedule(id);
  SaveSchedules(schedules);
  screen->DisplayMessage("Schedule deleted", true, true);
  request->redirect("/ScheduleList");
}

void handle_OnDownloadSchedules(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  request->send(SPIFFS, schedulesFile, "application/json", true);
}

void handle_UploadSchedules(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  static File uploadFile;
  if (!index)
  {
    displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
    screen->DisplayMessage("Uploading schedules", true, true);
    uploadFile = SPIFFS.open(schedulesFile, "w");
  }
  if (uploadFile)
  {
    uploadFile.write(data, len);
  }
  if (final)
  {
    if (uploadFile)
    {
      uploadFile.close();
    }
    InitializeSchedules();
    menu->GenerateIrrigationSubmenu();
    screen->DisplayMessage("Schedules uploaded", true, true);
  }
}

void handle_OnGetSettings(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle get settings", true, true);
  String jsonString = convertToJson(schedules);
  request->send(200, "text/html", jsonString);
  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnRoot(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Starting handle request", true, true);

  String setActive;
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (!digitalRead(schedules.getPin(i)))
    {
      Serial.println("Channel " + String(i + 1) + " is on");
      setActive += "document.getElementById('ch" + String(i + 1) + "').classList.add('button-check');";
    }
  }

  // Time left is filled in by the "status" SSE event sent on connect
  String modeStr = irrigationScheduleEnabled ? "Scheduled" : "Manual";

  request->send(SPIFFS, "/Index", "text/html", false,
    [setActive, modeStr](const String &var) -> String {
      if (var == "SET_ACTIVE") return setActive;
      if (var == "MODE")       return modeStr;
      return String();
    });

  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnToggleSwitch(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle toggle", true, true);
  int ch = request->hasArg("ch") ? request->arg("ch").toInt() - 1 : -1;
  int duration = request->hasArg("duration") ? request->arg("duration").toInt() : manualIrrigationDurationDef;
  if (ch < -1 || ch > 7)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    request->send(400, "application/json", "{\"error\": \"Invalid channel\"}");
    return;
  }
  if (duration < manualIrrigationDurationMin || duration > manualIrrigationDurationMax)
  {
    screen->DisplayMessage("Invalid duration", true, true);
    request->send(400, "application/json", "{\"error\": \"Invalid duration\"}");
    return;
  }

  toggleChannel(ch, duration);

  request->send(200, "application/json",
              "{\"status\": \"OK\", \"channel\": " + String(ch + 1) + "}");
  screen->DisplayMessage("Finished on: " + String(ch), true, true);
}

void handle_OnSetScheduled(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set scheduled", true, true);

  setScheduledMode(!irrigationScheduleEnabled);
  request->send(200, "application/json",
              irrigationScheduleEnabled ? "{\"status\": \"OK\", \"scheduled\": true}" : "{\"status\": \"OK\", \"scheduled\": false}");
  screen->DisplayMessage("Finished on set scheduled", true, true);
}

// Seconds left of the running channel (scheduled or manual), 0 when idle. Wrap-safe.
unsigned long remainingSeconds()
{
  unsigned long endTime = irrigationScheduleEnabled ? irrigationScheduledEnd : irrigationManualEnd;
  if (endTime == 0)
    return 0;
  long diff = (long)(endTime - millis());
  return diff > 0 ? diff / 1000 : 0;
}

// Relay states as a string of '0'/'1' per channel, e.g. "01000000"
String channelStates()
{
  String states;
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    states += digitalRead(schedules.getPin(i)) == LOW ? "1" : "0";
  }
  return states;
}

// Payload of the "status" SSE event: {"scheduled":true,"remaining":754}
String statusJson()
{
  JsonDocument doc;
  doc["scheduled"] = irrigationScheduleEnabled;
  doc["remaining"] = remainingSeconds();
  String json;
  serializeJson(doc, json);
  return json;
}

void sendStatusEvent()
{
  events.send(statusJson().c_str(), "status", millis());
}

void sendChannelEvent()
{
  events.send(channelStates().c_str(), "channels", millis());
}

void handle_NotFound(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Page not found", true, true);
  request->send(404, "text/plain", "Page not found");
}

// JSON API
// Mutations are POST with an application/json body only: AsyncCallbackJsonWebHandler rejects
// other content types, and a cross-origin page cannot send JSON without a CORS preflight.
void addJsonPostHandler(const char *uri, ArJsonRequestHandlerFunction fn, size_t maxContentLength)
{
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(uri, fn);
  handler->setMethod(HTTP_POST);
  handler->setMaxContentLength(maxContentLength);
  server.addHandler(handler);
}

void sendJson(AsyncWebServerRequest *request, const JsonDocument &doc, int code)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->setCode(code);
  serializeJson(doc, *response);
  request->send(response);
}

void sendJsonError(AsyncWebServerRequest *request, const char *message, int code)
{
  JsonDocument doc;
  doc["error"] = message;
  sendJson(request, doc, code);
}

void buildStateJson(JsonDocument &doc)
{
  doc["scheduled"] = irrigationScheduleEnabled;
  doc["remaining"] = remainingSeconds();
  JsonArray channels = doc["channels"].to<JsonArray>();
  JsonArray names = doc["names"].to<JsonArray>();
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    channels.add(digitalRead(schedules.getPin(i)) == LOW ? 1 : 0);
    names.add(i < (int)channelNames.size() ? channelNames[i] : String(i + 1));
  }
  doc["durationMin"] = manualStartDurationMin;
  doc["durationMax"] = manualIrrigationDurationMax;
  doc["durationDef"] = manualIrrigationDurationDef;
}

void sendState(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  buildStateJson(doc);
  sendJson(request, doc);
}

// Validates a schedule in the schedules.json format and fills `out`; returns an error or nullptr
const char *scheduleFromJson(JsonObjectConst obj, IrrigationSchedule &out)
{
  if (!obj["startTime"].is<int>() || obj["startTime"].as<int>() < 0 || obj["startTime"].as<int>() > 1439)
    return "Invalid start time";
  if (!obj["daysToRun"].is<int>() || obj["daysToRun"].as<int>() < 0 || obj["daysToRun"].as<int>() >= (int)daysToRunValues.size())
    return "Invalid days to run";
  if (!obj["weight"].is<int>() || obj["weight"].as<int>() < 0 || obj["weight"].as<int>() > 200)
    return "Invalid weight";
  JsonArrayConst durations = obj["channelDurations"];
  if (durations.isNull() || (int)durations.size() != irrigationChannelNumber)
    return "Invalid channel durations";
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (!durations[i].is<int>() || durations[i].as<int>() < manualIrrigationDurationMin || durations[i].as<int>() > manualIrrigationDurationMax)
      return "Invalid duration";
  }

  int startTime = obj["startTime"];
  out.setStartTime(startTime / 60, startTime % 60);
  out.setDaysToRun(obj["daysToRun"]);
  out.setWeight(obj["weight"]);
  for (int i = 0; i < irrigationChannelNumber; i++)
    out.addChannelDuration(i, durations[i]);
  return nullptr;
}

void handle_ApiState(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  sendState(request);
}

// {"ch": 1..8, "duration": minutes} toggles a channel; {"ch": 0} stops all
void handle_ApiChannel(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  int ch = json["ch"].is<int>() ? json["ch"].as<int>() : -1;
  if (ch < 0 || ch > irrigationChannelNumber)
    return sendJsonError(request, "Invalid channel");
  int duration = json["duration"].is<int>() ? json["duration"].as<int>() : manualIrrigationDurationDef;
  bool starting = ch > 0 && digitalRead(schedules.getPin(ch - 1)) == HIGH;
  if (starting && (duration < manualStartDurationMin || duration > manualIrrigationDurationMax))
    return sendJsonError(request, "Invalid duration");

  toggleChannel(ch - 1, duration);
  sendState(request);
}

// {"scheduled": true|false}
void handle_ApiMode(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  if (!json["scheduled"].is<bool>())
    return sendJsonError(request, "Invalid mode");
  setScheduledMode(json["scheduled"].as<bool>());
  sendState(request);
}

void handle_ApiSchedules(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  JsonDocument doc;
  JsonArray days = doc["days"].to<JsonArray>();
  for (const char *day : daysToRunValues)
    days.add(day);

  bool rtcValid = !rtc.lostPower();
  DateTime now = rtcValid ? rtc.now() : DateTime();
  JsonArray list = doc["schedules"].to<JsonArray>();
  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule sch = schedules.getSchedule(i);
    JsonObject obj = list.add<JsonObject>();
    obj["id"] = i;
    obj["startTime"] = sch.getStartTime();
    obj["daysToRun"] = sch.getDaysToRun();
    obj["weight"] = sch.getWeight();
    JsonArray durations = obj["channelDurations"].to<JsonArray>();
    for (int j = 0; j < irrigationChannelNumber; j++)
      durations.add(sch.getChannelDuration(j));
    // Which of today and the next 3 days the schedule runs on, e.g. "!-!-"
    String next;
    if (rtcValid)
    {
      for (int d = 0; d <= 3; ++d)
        next += sch.isValidForDay(now + TimeSpan(d, 0, 0, 0)) ? "!" : "-";
    }
    obj["next"] = next;
  }
  sendJson(request, doc);
}

// {"id": -1 (new) or index, "startTime", "daysToRun", "weight", "channelDurations": [8]}
void handle_ApiScheduleSave(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  int id = json["id"].is<int>() ? json["id"].as<int>() : -1;
  if (id < -1 || id >= schedules.getNumberOfSchedules())
    return sendJsonError(request, "Invalid schedule id");

  IrrigationSchedule schedule;
  const char *error = scheduleFromJson(json.as<JsonObjectConst>(), schedule);
  if (error)
    return sendJsonError(request, error);

  if (id >= 0)
  {
    schedules.updateSchedule(id, schedule);
  }
  else
  {
    schedules.addSchedule(schedule);
    id = schedules.getNumberOfSchedules() - 1;
  }
  SaveSchedules(schedules);
  screen->DisplayMessage("Schedule saved", true, true);

  JsonDocument doc;
  doc["id"] = id;
  sendJson(request, doc);
}

// {"id": index}
void handle_ApiScheduleDelete(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  int id = json["id"].is<int>() ? json["id"].as<int>() : -1;
  if (id < 0 || id >= schedules.getNumberOfSchedules())
    return sendJsonError(request, "Invalid schedule id");
  schedules.removeSchedule(id);
  SaveSchedules(schedules);
  screen->DisplayMessage("Schedule deleted", true, true);

  JsonDocument doc;
  doc["status"] = "OK";
  sendJson(request, doc);
}

// {"value": 0..255}
void handle_ApiDimming(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  int value = json["value"].is<int>() ? json["value"].as<int>() : -1;
  if (value < 0 || value > 255)
    return sendJsonError(request, "Invalid dimming");
  screen->DisplayDimm(value);

  JsonDocument doc;
  doc["value"] = value;
  sendJson(request, doc);
}

void handle_ApiExport(AsyncWebServerRequest *request)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", convertToJson(schedules));
  response->addHeader("Content-Disposition", "attachment; filename=\"schedules.json\"");
  request->send(response);
}

// Body is a schedules.json export. Every schedule is validated before anything is replaced.
void handle_ApiImport(AsyncWebServerRequest *request, JsonVariant &json)
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  JsonArrayConst list = json["schedules"];
  if (list.isNull())
    return sendJsonError(request, "Missing schedules");

  std::vector<IrrigationSchedule> imported;
  for (JsonObjectConst obj : list)
  {
    IrrigationSchedule schedule;
    const char *error = scheduleFromJson(obj, schedule);
    if (error)
      return sendJsonError(request, error);
    imported.push_back(schedule);
  }

  schedules.clearSchedules();
  for (const IrrigationSchedule &schedule : imported)
    schedules.addSchedule(schedule);
  SaveSchedules(schedules);
  screen->DisplayMessage("Schedules imported", true, true);

  JsonDocument doc;
  doc["count"] = (int)imported.size();
  sendJson(request, doc);
}

void handleTimer(bool runNow = false)
{
  if (runNow || millis() - TimerLastRun >= TIMER_TIMEOUT_INTERVAL)
  {
    TimerLastRun = millis();
    Serial.println("Timer handler reached");
    if (WiFi.status() != WL_CONNECTED)
    {
      screen->DisplayMessage("WiFi not connected, trying to reconnect", true, true);
      InitializeWiFi();
      if (WiFi.status() == WL_CONNECTED)
      {
        InitializeOTA();
        InitializeMQTT();
      }
      else
      {
        screen->DisplayMessage("Failed to reconnect WiFi", true, true);
      }
    }
    UpdateTimeZone();
    if (rtc.lostPower() || runNow)
    {
      struct tm timeinfo;
      if (WiFi.status() == WL_CONNECTED)
      {
        if (getLocalTime(&timeinfo, 200))
        {
          screen->DisplayMessage(runNow ? "Updating RTC from NTP" : "RTC lost power, setting time", true, true);
          rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        }
        else
        {
          screen->DisplayMessage("Failed to set RTC, retrying in 3 minutes", true, true);
          TimerLastRun = millis() - (TIMER_TIMEOUT_INTERVAL - 180000); // 3 minutes
        }
      }
      else
      {
        screen->DisplayMessage("Failed to set RTC", true, true);
      }
    }
  }
}

void handleRotary()
{
  byte rotation = menu->encoder.rotate();
  if (rotation != 0x00)
  {
    screen->DisplayActivate();
  }
  byte pushed = menu->encoder.push();
  if (pushed != 0x00)
  {
    isMenuActive = true;
    menu->menu.show();
  }
}

// MQTT Handlers
void sendMQTTMessage(String path, String payload)
{
  if (!mqttConfig.isValid())
  {
    screen->DisplayMessage("MQTT not configured", true, true);
    return;
  }
  if (!mqttClient.connected())
  {
    if (!mqttClient.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))
    {
      screen->DisplayMessage("MQTT connect failed", true, true);
      return;
    }
  }
  String topic = String(mqttConfig.topic) + path;
  if (!mqttClient.publish(topic.c_str(), payload.c_str()))
  {
    screen->DisplayMessage("MQTT send failed", true, true);
  }
}

void mqttMessageHandler(char *topic, byte *payload, unsigned int length)
{
  String t = String(topic);
  String pl;
  for (unsigned int i = 0; i < length; i++)
    pl += (char)payload[i];
  pl.trim();
  screen->DisplayMessage("MQTT recv: " + t + " -> " + pl, true, true);

  int idx = t.indexOf("/command/");
  if (idx < 0)
    return;
  String cmd = t.substring(idx + 9); // after /command/
  // Expected: channelN or all
  if (cmd.startsWith("channel"))
  {
    int ch = cmd.substring(7).toInt() - 1; // channel1 -> index 0
    if (ch < 0 || ch >= irrigationChannelNumber)
    {
      screen->DisplayMessage("Invalid MQTT channel", true, true);
      return;
    }
    if (pl.equalsIgnoreCase("OFF"))
    {
      stopChannel(ch);
    }
    else if (pl.startsWith("ON:") || pl.startsWith("on:"))
    {
      // ON:<minutes> — e.g. "ON:30" starts the channel for 30 minutes
      int duration = pl.substring(3).toInt();
      if (duration <= 0) duration = manualIrrigationDurationDef;
      startChannel(ch, duration);
    }
    else if (pl.equalsIgnoreCase("ON"))
    {
      startChannel(ch, manualIrrigationDurationDef);
    }
    else
    {
      // Plain number as payload is treated as duration in minutes
      int duration = pl.toInt();
      if (duration > 0)
        startChannel(ch, duration);
    }
  }
  else if (cmd.equalsIgnoreCase("OFF") && pl.equalsIgnoreCase("ON"))
  {
    stopChannel(-1);
    sendMQTTMessage("status/off", "off");
  }
  else if (cmd.equalsIgnoreCase("SCHEDULED"))
  {
    setScheduledMode(pl.equalsIgnoreCase("ON"));
  }
  else if (cmd.equalsIgnoreCase("STATUS"))
  {
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      String status = digitalRead(schedules.getPin(i)) == LOW ? "on" : "off";
      sendMQTTMessage("status/channel" + String(i + 1), status);
    }
    String scheduledStatus = irrigationScheduleEnabled ? "on" : "off";
    sendMQTTMessage("status/scheduled", scheduledStatus);
    sendMQTTMessage("status/status", "off");
  }
  else
  {
    screen->DisplayMessage("Unknown MQTT command", true, true);
  }
}

void loopMQTT()
{
  if (mqttConfig.isValid())
  {
    if (!mqttClient.connected())
    {
      if (mqttClient.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))
      {
        screen->DisplayMessage("MQTT connected", true, true);
        mqttSubscribed = false; // will subscribe below
      }
    }
    if (mqttClient.connected())
    {
      if (!mqttSubscribed)
      {
        // Subscribe to command topic: e.g., Irrigation/command/#
        String cmdTopic = String(mqttConfig.topic) + "command/#";
        mqttClient.subscribe(cmdTopic.c_str());
        mqttSubscribed = true;
      }
      mqttClient.loop();
    }
  }
}

// ##################################################################################################
// ##      ##       ###       ####      ###      ####      ###        ###      ###      ###  ####  ##
// ####  ####  ####  ##  ####  #####  ####  ########  ####  #####  ########  ####  ####  ##    ##  ##
// ####  ####       ###       ######  ####   #   ###        #####  ########  ####  ####  ##  #  #  ##
// ####  ####  ##  ####  ##  #######  ####  # ##  ##  ####  #####  ########  ####  ####  ##  ##    ##
// ##      ##  ####  ##  ####  ###      ###      ###  ####  #####  ######      ###      ###  ####  ##
// ##################################################################################################

// Irrigation
void SendIrrigationStatusUpdate()
{
  if (millis() - irrigationTimeLeftLastSend < 1000)
    return;

  unsigned long endTime = irrigationScheduleEnabled ? irrigationScheduledEnd : irrigationManualEnd;
  if (endTime == 0 || millis() > endTime)
    return;

  // Web clients count down locally; SSE "status" is only sent when the deadline changes
  irrigationTimeLeftLastSend = millis();
  sendMQTTMessage("status/timeLeft", String(remainingSeconds()));
}

void ManageScheduledChannels()
{
  if (millis() - irrigationLastCheck < irrigationCheckInterval)
    return;
  irrigationLastCheck = millis();

  DateTime now = rtc.now();
  unsigned long nowU = now.unixtime();
  int channelToStart = -1;
  unsigned long channelEndTimeU = 0;

  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule schedule = schedules.getSchedule(i);

    // Check both yesterday and today for schedules that might have started yesterday
    // and are still running today.
    for (int dayOffset = -1; dayOffset <= 0; dayOffset++)
    {
      DateTime checkDay = now + TimeSpan(86400 * dayOffset);
      if (!schedule.isValidForDay(checkDay))
        continue;

      DateTime dayStart(checkDay.year(), checkDay.month(), checkDay.day(), 0, 0, 0);
      unsigned long startTimeU = dayStart.unixtime() + schedule.getStartTime() * 60;

      for (int j = 0; j < schedules.getNumberOfChannels(); j++)
      {
        unsigned long endTimeU = startTimeU + (schedule.getChannelDuration(j) * schedule.getWeight() * 60 + 50) / 100;

        if (nowU >= startTimeU && nowU < endTimeU)
        {
          channelToStart = j;
          channelEndTimeU = endTimeU;
          break;
        }

        startTimeU = endTimeU;
      }

      if (channelToStart != -1)
        break;
    }

    if (channelToStart != -1)
      break;
  }

  bool channelsChanged = false;
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (i != channelToStart)
    {
      if (digitalRead(schedules.getPin(i)) == LOW)
      {
        digitalWrite(schedules.getPin(i), HIGH);
        channelsChanged = true;
        displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
        screen->DisplayMessage("Channel " + String(i + 1) + " stopped", true, true);
        sendMQTTMessage("status/channel" + String(i + 1), "off");
      }
    }
    else
    {
      if (digitalRead(schedules.getPin(i)) == HIGH)
      {
        digitalWrite(schedules.getPin(i), LOW);
        channelsChanged = true;
        displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
        screen->DisplayMessage("Channel " + String(i + 1) + " started", true, true);
        sendMQTTMessage("status/channel" + String(i + 1), "on");
      }
    }
  }

  bool statusChanged = channelsChanged;
  if (channelToStart != -1)
  {
    unsigned long newEnd = millis() + (channelEndTimeU - nowU) * 1000UL;
    // The deadline is recomputed every check; only notify web clients when it really moved
    // (new channel, edited schedule), not for sub-second jitter.
    if (irrigationScheduledEnd == 0 || abs((long)(newEnd - irrigationScheduledEnd)) > 2000)
      statusChanged = true;
    irrigationScheduledEnd = newEnd;
  }
  else if (irrigationScheduledEnd > 0)
  {
    sendMQTTMessage("status/timeLeft", "0");
    irrigationScheduledEnd = 0;
    statusChanged = true;
  }
  if (channelsChanged)
    sendChannelEvent();
  if (statusChanged)
    sendStatusEvent();
  irrigationManualEnd = 0;
}

void ManageManualChannels()
{
  if (irrigationManualEnd == 0 || millis() <= irrigationManualEnd)
    return;

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (digitalRead(schedules.getPin(i)) == LOW)
    {
      digitalWrite(schedules.getPin(i), HIGH);
      sendMQTTMessage("status/channel" + String(i + 1), "off");
    }
  }
  screen->DisplayMessage("Manual irrigation ended", true, true);
  sendMQTTMessage("status/manual", "off");
  sendMQTTMessage("status/timeLeft", "0");
  irrigationManualEnd = 0;
  sendChannelEvent();
  sendStatusEvent();
}

void ManageIrrigation()
{
  SendIrrigationStatusUpdate();
  if (irrigationScheduleEnabled)
    ManageScheduledChannels();
  else
    ManageManualChannels();
}

// Switches between scheduled and manual mode. A real switch stops every channel, so no valve
// keeps running without a deadline (a scheduled run has no manual end and vice versa).
void setScheduledMode(bool enabled)
{
  if (irrigationScheduleEnabled != enabled)
  {
    irrigationScheduleEnabled = enabled;
    irrigationScheduledEnd = 0;
    irrigationLastCheck = 0; // evaluate schedules right away
    stopChannel(-1);
    screen->DisplayMessage("Irrigation set to " + String(enabled ? "scheduled" : "manual") + " mode", true, true);
  }
  sendMQTTMessage("status/scheduled", enabled ? "on" : "off");
}

void startChannel(int channel, int duration)
{
  if (channel < 0 || channel >= irrigationChannelNumber)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    return;
  }
  irrigationScheduleEnabled = false;
  irrigationScheduledEnd = 0;
  sendMQTTMessage("status/scheduled", "off");

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (i != channel)
    {
      if (digitalRead(schedules.getPin(i)) == LOW)
      {
        digitalWrite(schedules.getPin(i), HIGH);
        sendMQTTMessage("status/channel" + String(i + 1), "off");
      }
    }
    else
    {
      digitalWrite(schedules.getPin(i), LOW);
      sendMQTTMessage("status/channel" + String(i + 1), "on");
    }
  }
  irrigationManualEnd = millis() + (unsigned long)duration * 60000UL;
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Channel " + String(channel + 1) + " started for " + String(duration) + " minutes", true, true);
  sendMQTTMessage("status/timeLeft", String((unsigned long)(duration) * 60));
  sendChannelEvent();
  sendStatusEvent();
}

void stopChannel(int channel)
{
  if (channel < -1 || channel >= irrigationChannelNumber)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    return;
  }
  if (channel == -1)
  {
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      if (digitalRead(schedules.getPin(i)) == LOW)
      {
        digitalWrite(schedules.getPin(i), HIGH);
        sendMQTTMessage("status/channel" + String(i + 1), "off");
      }
    }
    displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
    screen->DisplayMessage("All channels stopped", true, true);
    irrigationManualEnd = 0;
    sendMQTTMessage("status/timeLeft", "0");
    sendChannelEvent();
    sendStatusEvent();
    return;
  }

  digitalWrite(schedules.getPin(channel), HIGH);
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Channel " + String(channel + 1) + " stopped", true, true);
  sendMQTTMessage("status/channel" + String(channel + 1), "off");
  irrigationManualEnd = 0;
  sendMQTTMessage("status/timeLeft", "0");
  sendChannelEvent();
  sendStatusEvent();
}

void toggleChannel(int channel, int duration)
{
  Serial.printf("Channel %d toggle start\n", channel);
  screen->DisplayMessage("Handle toggle", true, true);
  if (channel == -1)
  {
    stopChannel(-1);
  }
  else if (digitalRead(schedules.getPin(channel)) == HIGH)
  {
    if (duration < manualIrrigationDurationMin || duration > manualIrrigationDurationMax)
    {
      screen->DisplayMessage("Invalid duration", true, true);
    }
    else
    {
      startChannel(channel, duration);
    }
  }
  else
  {
    stopChannel(channel);
  }

  exitMenuCallback();
}

// ###########################################################################
// ###########################################################################
// ##########      ###        ##        ##  ####  ##       ###################
// #########  ########  ###########  #####  ####  ##  ####  ##################
// ##########      ###        #####  #####  ####  ##  ####  ##################
// ###############  ##  ###########  #####  ####  ##       ###################
// ###############  ##  ###########  #####  ####  ##  ########################
// ##########      ###        #####  ######      ###  ########################
// ###########################################################################
// ###########################################################################
void setup()
{
  Serial.begin(115200);
  delay(100);

  Serial.println("Starting Irrigation Controller");

  schedules.setPins(relayPins);
  InicializeRelays();
  screen = new Display(relayPins, displayDimmPin);

  InitFS();

  InitializeWebServer();

  if (readWiFiCredentials())
  {
    InitializeWiFi();
  }
  else
  {
    screen->DisplayMessage("No wifi credentials", true, true);
  }

  InitializeRTC();

  InitializeSchedules();
  InitializeChannelNames();

  menu = new Menu();
  menu->renderer.begin();
  menu->GenerateIrrigationSubmenu();
  menu->GenerateManualScreen();
  menu->menu.setScreen(mainScreen, false);
  menu->menu.hide();
  menu->encoder.setSwitchDebounceDelay(50);

  if (WiFi.status() == WL_CONNECTED)
  {
    InitializeOTA();
    InitializeMQTT();
  }
  else
  {
    screen->DisplayMessage("OTA, MQTT not started", true, true);
  }
}

// ###########################################################################
// ###########################################################################
// #########  #########      ####     ####       #############################
// #########  ########  ####  ##  ####  ##  ####  ############################
// #########  ########  ####  ##  ####  ##  ####  ############################
// #########  ########  ####  ##  ####  ##       #############################
// #########  ########  ####  ##  ####  ##  ##################################
// #########        ###      ####      ###  ##################################
// ###########################################################################
// ###########################################################################

// Loop
void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    loopMQTT();
    ArduinoOTA.handle();
  }
  ManageIrrigation();
  if (isMenuActive)
  {
    screen->DisplayActivate();
    menu->rotaryInput.observe();
  }
  else
  {
    screen->DisplayText();
    handleRotary();
  }
  handleTimer();
}

// ###################################################################################
// ###      ####      ###  ########  ########       ####      ####      ####  ####  ##
// ##  ####  ##  ####  ##  ########  ########  ####  ##  ####  ##  ####  ###  ##  ####
// ##  ########        ##  ########  ########       ###        ##  #########    ######
// ##  ########  ####  ##  ########  ########  ####  ##  ####  ##  #########  #  #####
// ##  ####  ##  ####  ##  ########  ########  ####  ##  ####  ##  ####  ###  ##  ####
// ###      ###  ####  ##        ##        ##       ###  ####  ###      ####  ####  ##
// ###################################################################################

void exitMenuCallback()
{
  isMenuActive = false;
  menu->menu.hide();
  Serial.println("Exit menu");
}

void resetScreenCallback()
{
  screen->SetCustomChars();
  screen->DisplayActivate();
  exitMenuCallback();
}

void resetRtcCallback()
{
  handleTimer(true);
  exitMenuCallback();
}

void rebootCallback()
{
  exitMenuCallback();
  ESP.restart();
}

void wifiInformationCallback()
{
  exitMenuCallback();
  screen->DisplayMessage("SSID: " + wifiSsid, true, true);
  screen->DisplayMessage("IP: " + wifiIpAddress, true, true);
  screen->DisplayMessage("Hostname: " + wifiHostname, true, true);
}

void toggleChannelCallback(int channel)
{
  Serial.printf("Channel %d toggle start\n", channel);
  screen->DisplayMessage("Handle toggle", true, true);

  int duration = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<uint8_t> *>(manualScreen->getItemAt(0))->getWidgetAt(0))->getValue();
  toggleChannel(channel, duration);

  exitMenuCallback();
}

void setScheduledCallback()
{
  setScheduledMode(true);
  exitMenuCallback();
}

void commandScheduleSelectCallback(int scheduleIndex)
{
  Serial.printf("Selected schedule %d\n", scheduleIndex);
  menu->GenerateScheduleViewSubmenu(scheduleIndex);
  menu->menu.setScreen(scheduleViewScreen);
}

void commandScheduleEditCallback(int scheduleIndex)
{
  Serial.printf("Edit schedule %d\n", scheduleIndex);
  menu->GenerateScheduleEditSubmenu(scheduleIndex);
  menu->menu.setScreen(scheduleEditScreen);
}

void commandScheduleSaveCallback(int scheduleIndex)
{
  IrrigationSchedule schedule = scheduleIndex < 0 ? IrrigationSchedule() : schedules.getSchedule(scheduleIndex);

  int numberOfChannels = schedules.getNumberOfChannels();

  int hour = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<int, int> *>(scheduleEditScreen->getItemAt(0))->getWidgetAt(0))->getValue();
  int minute = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<int, int> *>(scheduleEditScreen->getItemAt(0))->getWidgetAt(1))->getValue();
  int daysToRun = static_cast<WidgetList<const char *> *>(static_cast<ItemWidget<uint8_t> *>(scheduleEditScreen->getItemAt(1))->getWidgetAt(0))->getValue();
  int weight = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<uint8_t> *>(scheduleEditScreen->getItemAt(2))->getWidgetAt(0))->getValue();
  schedule.setStartTime(hour, minute);
  schedule.setDaysToRun(daysToRun);
  schedule.setWeight(weight);
  Serial.printf("time: %02d:%02d, days: %d, weight: %d\n", hour, minute, daysToRun, weight);
  for (int i = 0; i < numberOfChannels; i++)
  {
    int duration = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<int> *>(scheduleEditScreen->getItemAt(3 + i))->getWidgetAt(0))->getValue();
    schedule.addChannelDuration(i, duration);
    Serial.printf("Channel %d duration: %d\n", i, duration);
  }

  if (scheduleIndex < 0)
  {
    schedules.addSchedule(schedule);
  }
  else
  {
    schedules.updateSchedule(scheduleIndex, schedule);
  }
  SaveSchedules(schedules);

  screen->DisplayMessage("Schedule saved", true, true);

  menu->menu.process(BACK);
  exitMenuCallback();
  Serial.printf("Save schedule %d\n", scheduleIndex);
}

void commandScheduleDeleteCallback(int scheduleIndex)
{
  schedules.removeSchedule(scheduleIndex);
  SaveSchedules(schedules);
  screen->DisplayMessage("Schedule deleted", true, true);
  menu->menu.process(BACK);
  exitMenuCallback();
  Serial.printf("Delete schedule %d\n", scheduleIndex);
}

void ShowScheduleDaysSubmenuCallback()
{
  Serial.println("Showing schedule days submenu");
  menu->GenerateScheduleDaysSubmenu();
  menu->menu.setScreen(scheduleDaysScreen);
}
