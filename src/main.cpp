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
Display *screen;
Menu *menu;
bool isMenuActive = false;
std::vector<int> relayPins = {RELAY_PIN_1, RELAY_PIN_2, RELAY_PIN_3, RELAY_PIN_4, RELAY_PIN_5, RELAY_PIN_6, RELAY_PIN_7, RELAY_PIN_8};

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

// time_t getLastSundayUTC(int year, int month, int hour)
// {
//   struct tm t = {};
//   t.tm_year = year - 1900;
//   t.tm_mon = month - 1;
//   t.tm_mday = 31;
//   t.tm_hour = hour;
//   t.tm_min = 0;
//   t.tm_sec = 0;

//   time_t ts = mktime(&t);
//   t = *gmtime(&ts); // Use gmtime to treat as UTC

//   while (t.tm_wday != DST_START_WEEKDAY)
//   {
//     ts -= 24 * 60 * 60;
//     t = *gmtime(&ts);
//   }

//   // Serial.printf("Last Sunday of %d-%02d at %02d:00 UTC is %s", year, month, hour, asctime(&t));
//   return ts;
// }

// bool DaylightSavingActive()
// {
//   time_t now;
//   time(&now);

//   struct tm *utc = gmtime(&now);
//   int year = utc->tm_year + 1900;

//   time_t dstStart = getLastSundayUTC(year, DST_START_MONTH, DST_START_HOUR);
//   time_t dstEnd = getLastSundayUTC(year, DST_END_MONTH, DST_END_HOUR);

//   bool isDstActive = (now >= dstStart && now < dstEnd);
//   // Serial.printf("DST active: %s\n", isDstActive ? "Yes" : "No");

//   return isDstActive;
// }

void UpdateTimeZone()
{
  setenv("TZ", tz, 1);
  tzset();
  Serial.printf("Időzóna beállítva: %s\n", tz);
  // int daylight = DaylightSavingActive() ? daylightOffset_sec : 0;

  // char cst[17] = {0};
  // char cdt[17] = "DST";
  // char tz[33] = {0};

  // if (gmtOffset_sec % 3600)
  // {
  //   sprintf(cst, "UTC%ld:%02u:%02u", gmtOffset_sec / 3600, abs((gmtOffset_sec % 3600) / 60), abs(gmtOffset_sec % 60));
  // }
  // else
  // {
  //   sprintf(cst, "UTC%ld", gmtOffset_sec / 3600);
  // }
  // if (daylight != 3600)
  // {
  //   long tz_dst = gmtOffset_sec - daylight;
  //   if (tz_dst % 3600)
  //   {
  //     sprintf(cdt, "DST%ld:%02u:%02u", tz_dst / 3600, abs((tz_dst % 3600) / 60), abs(tz_dst % 60));
  //   }
  //   else
  //   {
  //     sprintf(cdt, "DST%ld", tz_dst / 3600);
  //   }
  // }
  // sprintf(tz, "%s%s", cst, cdt);
  // setenv("TZ", tz, 1);
  // tzset();

  // Serial.printf("Current TZ: %s\n", tz ? tz : "Not set");
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
  int countDown = 10;
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

                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 screen->DisplayMessage("Updating " + type, true, true); })
      .onEnd([]()
             { screen->DisplayMessage("End", true, true); })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    screen->DisplayMessage("Progress: " + String((progress / (total / 100))) + "%", true, true);
                    // esp_task_wdt_reset();
                  })
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

void displayRtc()
{
  DateTime now = rtc.now();
  Serial.printf("%d, %04d-%02d-%02d %02d:%02d:%02d\n", now.dayOfTheWeek(), now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.printf("%.2fºC\n", rtc.getTemperature());
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
  // file.readStringUntil('\n').toCharArray(ssid, 32);
  // ssid[strlen(ssid) - 1] = '\0';
  // file.readStringUntil('\n').toCharArray(password, 64);
  // password[strlen(password) - 1] = '\0';
  String ssidString = file.readStringUntil('\n');
  String passwordString = file.readStringUntil('\n');
  file.close();

  ssidString.trim();
  passwordString.trim();

  ssid = strdup(ssidString.c_str());
  password = strdup(passwordString.c_str());
  Serial.println("Wifi read from file");
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
    if (!server.hasArg("duration" + String(i + 1)) || server.arg("duration" + String(i + 1)).length() == 0 || server.arg("duration" + String(i + 1)).toInt() < 0 || server.arg("duration" + String(i + 1)).toInt() > 60)
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
  menu->GenerateIrrigationSubmenu();
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
      lines += string(sch.getStartTimeString().c_str());
      lines += "</div>";
      lines += "<div class=\"listItem\">";
      lines += std::to_string(sch.getWeight());
      lines += "</div>";
      lines += "<div class=\"listItem\">";
      lines += "<a href=\"Schedule?id=" + std::to_string(i) + "\" class=\"linkButton\">Details</a>";
      lines += "</div>";
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
        body.replace("{{SetActive}}", "document.getElementById('ch" + String(i + 1) + "').classList.add('button-check');");
      }
      file.close();
    }
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
  if (ch < -1 || ch > 7)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    server.send(400, "application/json", "{\"error\": \"Invalid channel\"}");
    return;
  }

  if (ch == -1)
  {
    irrigationScheduleEnabled = true;
    server.send(200, "application/json", "{\"status\": \"OK\", \"mode\": \"scheduled\", \"enabled\": " + String(irrigationScheduleEnabled) + "}");
    screen->DisplayMessage("Finished on: all", true, true);
    return;
  }

  irrigationScheduleEnabled = false;
  int pin = schedules.getPin(ch);

  if (digitalRead(pin) == HIGH)
  {
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
      digitalWrite(schedules.getPin(i), HIGH);
    }
    digitalWrite(pin, LOW);
  }
  else
  {
    digitalWrite(pin, HIGH);
  }
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  server.send(200, "application/json",
              "{\"status\": \"OK\", \"channel\": " + String(ch + 1) + ", \"pin\": " + String(pin) + "}");
  screen->DisplayMessage("Finished on: " + String(ch), true, true);
}

void handle_NotFound()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Page not found", true, true);
  server.send(404, "text/plain", "Page not found");
}

void handleTimer()
{
  if (timerTimeout-- <= 0)
  {
    Serial.println("Timer timeout reached");
    timerTimeout = TIMER_TIMEOUT_INTERVAL;
    UpdateTimeZone();
    if (rtc.lostPower())
    {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 200))
      {
        screen->DisplayMessage("RTC lost power, setting time", true, true);
        rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
      }
      else
      {
        screen->DisplayMessage("Failed to set RTC", true, true);
        timerTimeout = 1000; // Retry in 1000 ms
      }
    }
    else
    {
      displayRtc();
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
  // return if interval not reached
  if (!irrigationScheduleEnabled || !wifiConnected || millis() - irrigationLastCheck < irrigationCheckInterval)
  {
    return;
  }
  irrigationLastCheck = millis();

  // time_t now;
  // struct tm timeinfo;
  // time(&now);
  // localtime_r(&now, &timeinfo);
  // int currentTime = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  DateTime now = rtc.now();
  int currentTime = (now.hour() * 3600) + (now.minute() * 60) + now.second();

  int channelToStart = -1;
  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule schedule = schedules.getSchedule(i);
    int startTime = schedule.getStartTime() * 60;
    for (int j = 0; j < schedules.getNumberOfChannels(); j++)
    {
      int endTime = startTime + 60 * schedule.getChannelDuration(j);
      if (currentTime >= startTime && currentTime < endTime)
      {
        channelToStart = j;
        break;
      }
      startTime = endTime;
    }
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
      }
    }
    else
    {
      if (digitalRead(schedules.getPin(i)) == HIGH)
      {
        digitalWrite(schedules.getPin(i), LOW);
        displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
        screen->DisplayMessage("Channel " + String(i + 1) + " started", true, true);
      }
    }
  }
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

  Serial.println("Starting menu");
  menu = new Menu();
  menu->renderer.begin();
  menu->GenerateIrrigationSubmenu();
  menu->GenerateManualScreen();
  menu->menu.setScreen(mainScreen);
  menu->menu.hide();
  Serial.println("Menu started");

  if (wifiConnected)
  {
    InitializeOTA();
  }
  else
  {
    screen->DisplayMessage("OTA not started", true, true);
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
  if (Serial.available())
  {
    char c = Serial.read();
    int cInt = c - '0';
    // digitalWrite(relayPins[cInt], !digitalRead(relayPins[cInt]));
    analogWrite(5, cInt * 50);
  }

  ManageIrrigation();
  if (isMenuActive)
  {
    menu->rotaryInput.observe();
  }
  else
  {
    screen->DisplayText();
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
  if (wifiConnected)
  {
    server.handleClient();
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

void backlightCallback(int value)
{
  Serial.printf("Backlight: %d\n", value);
  screen->DisplayDimm(value);
}

void wifiInformationCallback()
{
  exitMenuCallback();
  screen->DisplayMessage("SSID: " + wifiSsid, true, true);
  screen->DisplayMessage("IP: " + wifiIpAddress, true, true);
  screen->DisplayMessage("Hostname: " + wifiHostname, true, true);
}

void toggleChannel(int channel)
{
  Serial.printf("Channel %d toggle start\n", channel);
  screen->DisplayMessage("Handle toggle", true, true);
  if (channel < -1 || channel > 7)
  {
    screen->DisplayMessage("Invalid channel", true, true);
    Serial.println("Invalid channel");
    return;
  }

  if (channel == -1)
  {
    irrigationScheduleEnabled = true;
    screen->DisplayMessage("Finished on: all", true, true);
    Serial.println("All channels off, schedule enabled");
  }
  else
  {
    irrigationScheduleEnabled = false;
    int pin = schedules.getPin(channel);

    if (digitalRead(pin) == HIGH)
    {
      for (int i = 0; i < irrigationChannelNumber; i++)
      {
        digitalWrite(schedules.getPin(i), HIGH);
      }
      digitalWrite(pin, LOW);
    }
    else
    {
      digitalWrite(pin, HIGH);
    }
    displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
    screen->DisplayMessage("Finished on: " + String(channel), true, true);
    Serial.printf("Channel %d toggled\n", channel);
  }

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

  menu->menu.process(BACK);
  exitMenuCallback();
  Serial.printf("Save schedule %d\n", scheduleIndex);
}

void commandScheduleDeleteCallback(int scheduleIndex)
{
  schedules.removeSchedule(scheduleIndex);
  SaveSchedules(schedules);
  menu->menu.process(BACK);
  exitMenuCallback();
  Serial.printf("Delete schedule %d\n", scheduleIndex);
}
