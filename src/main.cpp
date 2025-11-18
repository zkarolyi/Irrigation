#include <WiFi.h>
#include <WebServer.h>
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

float temperature, humidity, pressure, altitude;

const char *ssid;
const char *password;

RTC_DS3231 rtc;

WebServer server(80);
bool wifiConnected = false;
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

void UpdateTimeZone()
{
  setenv("TZ", tz, 1);
  tzset();
  Serial.printf("Időzóna beállítva: %s\n", tz);
}

bool InitializeWiFi()
{
  screen->DisplayMessage("Connecting to ");
  screen->DisplayMessage(ssid, false, true);
  Serial.println("Connect to router");

  // connect to your local wi-fi network
  WiFi.disconnect();
  WiFi.hostname(HOSTNAME);
  WiFi.begin(ssid, password);

  screen->DisplayMessage(".", true, false);
  // check wi-fi is connected to wi-fi network
  int countDown = 20;
  while (WiFi.status() != WL_CONNECTED && countDown-- > 0)
  {
    delay(1000);
    screen->DisplayMessage(".", false, false);
  }
  if (countDown > 0)
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
    InitializeWebServer();
    return true;
  }
  screen->DisplayMessage("WI-FI connection failed", true, true);
  screen->DisplayMessage("HTTP server not started", true, true);
  return false;
}

void InitializeWebServer()
{
  server.on("/", handle_OnRoot);
  server.on("/ToggleSwitch", HTTP_GET, handle_OnToggleSwitch);
  server.on("/SetScheduled", HTTP_GET, handle_OnSetScheduled);
  server.on("/Schedule", HTTP_GET, handle_OnGetSchedule);
  server.on("/Schedule", HTTP_POST, handle_OnSetSchedule);
  server.on("/SetDimming", HTTP_GET, handle_OnSetDimming);
  server.on("/ScheduleList", HTTP_GET, handle_onScheduleList);
  server.on("/GetSettings", HTTP_GET, handle_OnGetSettings);
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(handle_NotFound);

  server.begin();
  screen->DisplayMessage("HTTP server started", true, true);
}

void InitializeOTA()
{
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
  mqttClient.setServer(mqttConfig.broker, mqttConfig.port);
  // Set callback for incoming MQTT messages (commands)
  mqttClient.setCallback(mqttMessageHandler);
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
  ssid = newSsid;
  password = newPassword;
  Serial.printf("Saving wifi credentials: %s, %s\n", ssid, password);
  File file = SPIFFS.open("/wifi.txt", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println(ssid);
  file.println(password);
  file.close();

  Serial.println("Wifi credentials saved");
  wifiConnected = InitializeWiFi();
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
  String ssidString = file.readStringUntil('\n');
  String passwordString = file.readStringUntil('\n');
  file.close();
  if (ssid != nullptr)
  {
    free((void *)ssid);
    ssid = nullptr;
  }
  if (password != nullptr)
  {
    free((void *)password);
    password = nullptr;
  }

  ssidString.trim();
  passwordString.trim();

  ssid = strdup(ssidString.c_str());
  password = strdup(passwordString.c_str());
  Serial.println("Wifi read from file");
  return true;
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
void handle_OnSetDimming()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set dimming", true, true);
  int dimm = server.hasArg("dimm") ? server.arg("dimm").toInt() : 255;
  if (dimm < 0 || dimm > 255)
  {
    screen->DisplayMessage("Invalid dimming", true, true);
    server.send(400, "text/html", "Invalid dimming");
    return;
  }
  screen->DisplayDimm(dimm);
  server.send(200, "text/html", "OK (" + String(dimm) + ")");
  screen->DisplayMessage("Finished on: " + String(dimm), true, true);
}

void handle_OnGetSchedule()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle get schedule", true, true);
  String body;
  File file = SPIFFS.open("/Schedule", "r");
  if (file)
  {
    body = file.readString();
    file.close();
  }
  else
  {
    body = "Failed to open file";
  }

  int id = server.hasArg("id") ? server.arg("id").toInt() : -1;
  if (id < 0 || id >= schedules.getNumberOfSchedules())
  {
    // New schedule
    body.replace("${id}", "-1");
    for (int i = 0; i < 24; i++)
    {
      body.replace("${sTH" + String(i) + "}", "");
    }
    for (int i = 0; i < 60; i++)
    {
      body.replace("${sTM" + String(i) + "}", "");
    }
    int dtr = 0;
    for (int i = 0; i <= daysToRunValues.size(); i++)
    {
      body.replace("${dTR" + String(i) + "}", dtr == i ? " selected" : "");
    }
    int weight = 100;
    for (int i = 50; i <= 150; i = i + 25)
    {
      body.replace("${W" + String(i) + "}", weight == i ? " selected" : "");
    }
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      body.replace("${duration" + String(i + 1) + "}", "0");
    }
  }
  else
  {
    // Edit schedule
    body.replace("${id}", String(id));
    IrrigationSchedule sch = schedules.getSchedule(id);
    int sth = sch.getStartTimeHours();
    for (int i = 0; i < 24; i++)
    {
      body.replace("${sTH" + String(i) + "}", sth == i ? " selected" : "");
    }
    int stm = sch.getStartTimeMinutes();
    for (int i = 0; i < 60; i++)
    {
      body.replace("${sTM" + String(i) + "}", stm == i ? " selected" : "");
    }
    int dtr = (int)sch.getDaysToRun();
    screen->DisplayMessage("R:" + String(dtr), true, false);
    for (int i = 0; i <= daysToRunValues.size(); i++)
    {
      body.replace("${dTR" + String(i) + "}", dtr == i ? " selected" : "");
    }
    int weight = sch.getWeight();
    for (int i = 50; i <= 150; i = i + 25)
    {
      body.replace("${W" + String(i) + "}", weight == i ? " selected" : "");
    }
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      int cd = sch.getChannelDuration(i);
      body.replace("${duration" + String(i + 1) + "}", String(cd));
    }
  }

  server.send(200, "text/html", body);
  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnSetSchedule()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set schedule", true, true);
  int id = server.hasArg("id") ? server.arg("id").toInt() : -1;
  int getStartTime = server.hasArg("startTimeHour") && server.hasArg("startTimeMinute") ? server.arg("startTimeHour").toInt() * 60 + server.arg("startTimeMinute").toInt() : -1;
  if (getStartTime < 0 || getStartTime > 1440)
  {
    screen->DisplayMessage("Invalid start time", true, true);
    server.send(400, "text/html", "Invalid start time");
    return;
  }

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    String argName = "duration" + String(i + 1);
    if (!server.hasArg(argName) || server.arg(argName).length() == 0 || server.arg(argName).toInt() < manualIrrigationDurationMin || server.arg(argName).toInt() > manualIrrigationDurationMax)
    {
      screen->DisplayMessage("Invalid duration", true, true);
      server.send(400, "text/html", "Invalid duration" + String(i + 1));
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
    currSchedule.addChannelDuration(i, server.arg("duration" + String(i + 1)).toInt());
  }
  currSchedule.setDaysToRun(server.arg("daysToRun").toInt());
  currSchedule.setWeight(server.arg("weight").toInt());

  if (id >= 0 && id < schedules.getNumberOfSchedules())
  {
    schedules.updateSchedule(id, currSchedule);
  }
  else
  {
    schedules.addSchedule(currSchedule);
  }
  SaveSchedules(schedules);
  server.sendHeader("Location", "/ScheduleList", true);
  server.send(303, "text/plain", "");
  screen->DisplayMessage("Finished.", true, true);
}

void handle_onScheduleList()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle schedule list", true, true);
  String body;
  File file = SPIFFS.open("/ScheduleList", "r");
  if (file)
  {
    body = file.readString();
    string lines;
    for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
    {
      IrrigationSchedule sch = schedules.getSchedule(i);
      lines += "<div class=\"listItem\">";
      lines += string(sch.getStartTimeString().c_str()) + " ";
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
      lines += std::to_string(sch.getWeight());
      lines += "</div>";
      lines += "<div class=\"listItem\">";
      lines += "<a href=\"Schedule?id=" + std::to_string(i) + "\" class=\"linkButton\">Details</a>";
      lines += "</div>\n";
    }
    body.replace("${schedules}", lines.c_str());
    file.close();
  }
  else
  {
    body = "Failed to open file";
  }

  server.send(200, "text/html", body);
  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnGetSettings()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle get settings", true, true);
  String jsonString = convertToJson(schedules);
  server.send(200, "text/html", jsonString);
  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnRoot()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Starting handle request", true, true);

  String body;
  File file = SPIFFS.open("/Index", "r");
  if (file)
  {
    body = file.readString();
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      if (!digitalRead(schedules.getPin(i)))
      {
        Serial.println("Channel " + String(i + 1) + " is on");
        body.replace("<!--SetActive-->", "document.getElementById('ch" + String(i + 1) + "').classList.add('button-check');");
      }
      file.close();
    }
    String modeStr = irrigationScheduleEnabled ? "Scheduled" : "Manual";
    if (irrigationManualEnd > 0 && !irrigationScheduleEnabled && millis() < irrigationManualEnd)
    {
      unsigned long left = (irrigationManualEnd - millis()) / 1000;
      char timeLeft[6];
      snprintf(timeLeft, sizeof(timeLeft), "%02u:%02u", left / 60, left % 60);
      modeStr += " (" + String(timeLeft) + " left)";
    }
    body.replace("<!--Mode-->", modeStr.c_str());
  }
  else
  {
    body = "Failed to open file";
  }

  server.send(200, "text/html", body);
  screen->DisplayMessage("Finished.", true, true);
}

void handle_OnToggleSwitch()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle toggle", true, true);
  int ch = server.hasArg("ch") ? server.arg("ch").toInt() - 1 : -1;
  int duration = server.hasArg("duration") ? server.arg("duration").toInt() : manualIrrigationDurationDef;
  if (ch < -1 || ch > 7)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    server.send(400, "application/json", "{\"error\": \"Invalid channel\"}");
    return;
  }
  if (duration < manualIrrigationDurationMin || duration > manualIrrigationDurationMax)
  {
    screen->DisplayMessage("Invalid duration", true, true);
    server.send(400, "application/json", "{\"error\": \"Invalid duration\"}");
    return;
  }

  toggleChannel(ch, duration);

  server.send(200, "application/json",
              "{\"status\": \"OK\", \"channel\": " + String(ch + 1) + "}");
  screen->DisplayMessage("Finished on: " + String(ch), true, true);
}

void handle_OnSetScheduled()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle set scheduled", true, true);

  irrigationScheduleEnabled = true;
  server.send(200, "application/json",
              "{\"status\": \"OK\", \"scheduled\": true}");
  screen->DisplayMessage("Finished on set scheduled", true, true);
  sendMQTTMessage("status/scheduled", "on");
}

void handle_NotFound()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Page not found", true, true);
  server.send(404, "text/plain", "Page not found");
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
      wifiConnected = InitializeWiFi();
      if (!wifiConnected)
      {
        screen->DisplayMessage("Failed to reconnect WiFi", true, true);
      }
    }
    UpdateTimeZone();
    if (rtc.lostPower())
    {
      struct tm timeinfo;
      if (wifiConnected)
      {
        if (getLocalTime(&timeinfo, 200))
        {
          screen->DisplayMessage("RTC lost power, setting time", true, true);
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
    if (pl.equalsIgnoreCase("ON"))
    {
      int duration = manualIrrigationDurationDef;
      startChannel(ch, duration);
    }
    else if (pl.equalsIgnoreCase("OFF"))
    {
      stopChannel(ch);
    }
    else
    {
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
    irrigationScheduleEnabled = pl.equalsIgnoreCase("ON");
    screen->DisplayMessage("Irrigation set to scheduled mode", true, true);
    sendMQTTMessage("status/scheduled", pl.equalsIgnoreCase("ON") ? "on" : "off");
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
void ManageIrrigation()
{
  if (irrigationScheduleEnabled)
  {
    if (millis() - irrigationLastCheck < irrigationCheckInterval)
    {
      return;
    }
    irrigationLastCheck = millis();

    DateTime now = rtc.now();
    unsigned long nowU = now.unixtime();
    int channelToStart = -1;

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

    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      if (i != channelToStart)
      {
        if (digitalRead(schedules.getPin(i)) == LOW)
        {
          digitalWrite(schedules.getPin(i), HIGH);
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
          displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
          screen->DisplayMessage("Channel " + String(i + 1) + " started", true, true);
          sendMQTTMessage("status/channel" + String(i + 1), "on");
        }
      }
    }
    irrigationManualEnd = 0;
  }
  else
  {
    if (irrigationManualEnd > 0 && millis() > irrigationManualEnd)
    {
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
      irrigationManualEnd = 0;
    }
  }
}

void startChannel(int channel, int duration)
{
  if (channel < 0 || channel >= irrigationChannelNumber)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    return;
  }
  irrigationScheduleEnabled = false;
  sendMQTTMessage("status/scheduled", "off");

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (i != channel)
    {
      if(digitalRead(schedules.getPin(i)) == LOW)
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
  irrigationManualEnd = millis() + duration * 60 * 1000;
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Channel " + String(channel + 1) + " started for " + String(duration) + " minutes", true, true);
}

void stopChannel(int channel)
{
  if (channel < -1 || channel >= irrigationChannelNumber)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    return;
  }
  if(channel == -1)
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
    return;
  } 

  digitalWrite(schedules.getPin(channel), HIGH);
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Channel " + String(channel + 1) + " stopped", true, true);
  sendMQTTMessage("status/channel" + String(channel + 1), "off");
}

void toggleChannel(int channel, int duration)
{
  Serial.printf("Channel %d toggle start\n", channel);
  screen->DisplayMessage("Handle toggle", true, true);
  if(channel == -1)
  {
    stopChannel(-1);
  } else if(digitalRead(schedules.getPin(channel)) == HIGH)
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

  if (readWiFiCredentials())
  {
    wifiConnected = InitializeWiFi();
  }
  else
  {
    screen->DisplayMessage("No wifi credentials", true, true);
  }

  InitializeRTC();

  InitializeSchedules();

  menu = new Menu();
  menu->renderer.begin();
  menu->GenerateIrrigationSubmenu();
  menu->GenerateManualScreen();
  menu->menu.setScreen(mainScreen, false);
  menu->menu.hide();
  menu->encoder.setSwitchDebounceDelay(50);

  if (wifiConnected)
  {
    InitializeOTA();
    if (readMqttCredentials() && mqttConfig.isValid())
    {
      InitializeMQTT();
    }
    else
    {
      screen->DisplayMessage("No MQTT credentials", true, true);
    }
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
  if (WiFi.status() == WL_CONNECTED)
  {
    server.handleClient();
    loopMQTT();
    ArduinoOTA.handle();
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

void setScheduledCallback(){
  irrigationScheduleEnabled = true;
  sendMQTTMessage("status/scheduled", "on");
  screen->DisplayMessage("Irrigation set to scheduled mode", true, true);
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
