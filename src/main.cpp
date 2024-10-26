#include <WiFi.h>
#include <WebServer.h>
#include <main.h>
#include "SPIFFS.h"
#include <ArduinoOTA.h>
#include <Arduino.h>
#include "Irrigation.h"
#include <ArduinoJson.h>
#include <cmath>
#include <ESP32RotaryEncoder.h>
#include "display.h"
#include "menu.h"
using namespace std;

float temperature, humidity, pressure, altitude;

/*Put your SSID & Password*/
const char *ssid = "Csepp2";       // Enter SSID here
const char *password = "Karolyi1"; // Enter Password here

vector<String> v;

WebServer server(80);
IrrigationSchedules schedules(relayPins);
Display *screen;
Menu *menu;

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
    screen->DisplayMessage("Failed to open schedules", 2, true, true);
    return;
  }

  String json = file.readString();
  file.close();

  convertFromJson(json, schedules);
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
    screen->DisplayMessage("Failed to open file for writing", 2, true, true);
    return;
  }

  String json = convertToJson(schedules);
  if (file.write((uint8_t *)json.c_str(), json.length()) != json.length())
  {
    screen->DisplayMessage("Failed to write schedules", 2, true, true);
  }

  file.close();
}

void InitFS()
{
  if (!SPIFFS.begin(true))
  {
    screen->DisplayMessage("An Error has occurred while mounting SPIFFS", 0, true, true);
    return;
  }
}

void GetFile(String fileName)
{
  File file = SPIFFS.open(fileName);
  if (!file)
  {
    screen->DisplayMessage("Failed to open file for reading", 0, true, true);
    return;
  }

  // vector<String> v;
  while (file.available())
  {
    v.push_back(file.readStringUntil('\n'));
  }

  file.close();
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
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle set dimming", 0, true, true);
  int dimm = server.hasArg("dimm") ? server.arg("dimm").toInt() : 255;
  if (dimm < 0 || dimm > 255)
  {
    screen->DisplayMessage("Invalid dimming", 1, true, true);
    server.send(400, "text/html", "Invalid dimming");
    return;
  }
  screen->DisplayDimm(dimm);
  server.send(200, "text/html", "OK (" + String(dimm) + ")");
  screen->DisplayMessage("Finished on: " + String(dimm), 1, true, true);
}

void handle_OnGetSchedule()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle get schedule", 0, true, true);
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
    IrrigationDaysToRun dtr = IrrigationDaysToRun::All;
    for (int i = (int)IrrigationDaysToRun::All; i <= (int)IrrigationDaysToRun::Every7days; i++)
    {
      body.replace("${dTR" + String(i) + "}", dtr == (IrrigationDaysToRun)i ? " selected" : "");
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
    screen->DisplayMessage("R:" + String(dtr), 2, true, false);
    for (int i = (int)IrrigationDaysToRun::All; i <= (int)IrrigationDaysToRun::Every7days; i++)
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
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnSetSchedule()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle set schedule", 0, true, true);
  int id = server.hasArg("id") ? server.arg("id").toInt() : -1;
  int getStartTime = server.hasArg("startTimeHour") && server.hasArg("startTimeMinute") ? server.arg("startTimeHour").toInt() * 60 + server.arg("startTimeMinute").toInt() : -1;
  if (getStartTime < 0 || getStartTime > 1440)
  {
    screen->DisplayMessage("Invalid start time", 1, true, true);
    server.send(400, "text/html", "Invalid start time");
    return;
  }

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    if (!server.hasArg("duration" + String(i + 1)) || server.arg("duration" + String(i + 1)).length() == 0 || server.arg("duration" + String(i + 1)).toInt() < 0 || server.arg("duration" + String(i + 1)).toInt() > 60)
    {
      screen->DisplayMessage("Invalid duration", 1, true, true);
      server.send(400, "text/html", "Invalid duration" + String(i + 1));
      return;
    }
  }

  // startTimeHour=5&startTimeMinute=0&daysToRun=Saturday&weight=100&duration1=0&duration2=3&duration3=0&duration4=0&duration5=0&duration6=0&duration7=0&duration8=0

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
  currSchedule.setDaysToRun((IrrigationDaysToRun)server.arg("daysToRun").toInt());
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
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_onScheduleList()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle schedule list", 0, true, true);
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
      lines += std::to_string(sch.getStartTimeHours()) + ":" + std::to_string(sch.getStartTimeMinutes());
      lines += "</div>";
      lines += "<div class=\"listItem\">";
      lines += std::to_string(sch.getWeight());
      lines += "</div>";
      lines += "<div class=\"listItem\">";
      // lines += "<button class=\"linkButton\" onclick=\"//Schedule?id=" + std::to_string(i) + "\">Details</button>";
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
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnGetSettings()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle get settings", 0, true, true);
  String jsonString = convertToJson(schedules);
  server.send(200, "text/html", jsonString);
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnRoot()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Starting handle request", 0, true, true);
  bool ch[irrigationChannelNumber];
  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    ch[i] = digitalRead(schedules.getPin(i));
  }
  String body = GetHtml(ch);
  server.send(200, "text/html", body);
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnToggleSwitch()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Handle toggle", 0, true, true);
  int ch = server.hasArg("ch") ? server.arg("ch").toInt() - 1 : -1;
  if (ch < -1 || ch > 7)
  {
    screen->DisplayMessage("Invalid channel", 1, true, true);
    server.send(400, "application/json", "{\"error\": \"Invalid channel\"}");
    return;
  }

  if (ch == -1)
  {
    irrigationScheduleEnabled = true;
    server.send(200, "application/json", "{\"status\": \"OK\", \"mode\": \"scheduled\", \"enabled\": " + String(irrigationScheduleEnabled) + "}");
    screen->DisplayMessage("Finished on: all", 1, true, true);
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
  displayOutChange = displayTimeoutInterval;
  server.send(200, "application/json",
              "{\"status\": \"OK\", \"channel\": " + String(ch + 1) + ", \"pin\": " + String(pin) + "}");
  screen->DisplayMessage("Finished on: " + String(ch), 1, true, true);
}

void handle_NotFound()
{
  displayNetworkActivity = displayTimeoutInterval;
  screen->DisplayMessage("Page not found", 0, true, true);
  server.send(404, "text/plain", "Page not found");
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
  if (!irrigationScheduleEnabled || millis() - irrigationLastCheck < irrigationCheckInterval)
  {
    return;
  }
  irrigationLastCheck = millis();

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  int currentTime = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule schedule = schedules.getSchedule(i);
    int startTime = schedule.getStartTime() * 60;
    for (int j = 0; j < schedule.getNumberOfChannels(); j++)
    {
      int endTime = startTime + 60 * schedule.getChannelDuration(j);
      if (currentTime >= startTime && currentTime < endTime)
      {
        if (digitalRead(schedules.getPin(j)) == HIGH)
        {
          digitalWrite(schedules.getPin(j), LOW);
          displayOutChange = displayTimeoutInterval;
          screen->DisplayMessage("Channel " + String(j + 1) + " started", 2, true, true);
          Serial.printf("Channel %d started\n", j + 1);
        }
      }
      else
      {
        if (digitalRead(schedules.getPin(j)) == LOW)
        {
          digitalWrite(schedules.getPin(j), HIGH); // Deactivate the valve
          displayOutChange = displayTimeoutInterval;
          screen->DisplayMessage("Channel " + String(j + 1) + " stopped", 2, true, true);
          Serial.printf("Channel %d stopped\n", j + 1);
        }
      }
      startTime = endTime;
    }
  }
}

String GetHtml(bool ch[])
{
  String ptr;
  for (String s : v)
  {
    ptr += s;
  }

  for (int i = 0; i < irrigationChannelNumber; i++)
  {
    ptr.replace("${ch" + String(i + 1) + "_state}", ch[i] ? "on" : "off");
  }

  return ptr;
}

String SendHTML(float temperature, float humidity, float pressure, float altitude)
{
  String ptr;
  for (String s : v)
  {
    ptr += s;
  }

  return ptr;
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

  // InitializeLCD();
  InicializeRelays();
  menu = new Menu( rotaryEncoderPin1, rotaryEncoderPin2, rotaryEncoderButton);
  screen = new Display(relayPins, displayDimmPin);

  screen->DisplayMessage("Connecting to ", 0);
  screen->DisplayMessage(ssid, 0, false, true);

  // connect to your local wi-fi network
  WiFi.begin(ssid, password);

  screen->DisplayMessage(".", 1, true, false);
  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    screen->DisplayMessage(".", 1, false, false);
  }
  screen->DisplayMessage("WI-FI connected", 0, true, true);
  screen->DisplayMessage("IP address: ", 1);
  screen->DisplayMessage(WiFi.localIP().toString(), 1, false, true);

  InitFS();
  GetFile("/Index");
  InitializeSchedules();

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
  screen->DisplayMessage("HTTP server started", 0, true, true);
  // lcd.write(0);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
                 screen->DisplayMessage("Updating " + type, 0, true, true); })
      .onEnd([]()
             { screen->DisplayMessage("End", 0, true, true); })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    screen->DisplayMessage("Progress: " + String((progress / (total / 100))) + "%", 1, true, true);
                    // esp_task_wdt_reset();
                  })
      .onError([](ota_error_t error)
               {
                 screen->DisplayMessage("Error[" + String(error) + "]: ", 0, true, true);
                 if (error == OTA_AUTH_ERROR) screen->DisplayMessage("Auth Failed", 1, false, true);
                 else if (error == OTA_BEGIN_ERROR) screen->DisplayMessage("Begin Failed", 1, false, true);
                 else if (error == OTA_CONNECT_ERROR) screen->DisplayMessage("Connect Failed", 1, false, true);
                 else if (error == OTA_RECEIVE_ERROR) screen->DisplayMessage("Receive Failed", 1, false, true);
                 else if (error == OTA_END_ERROR) screen->DisplayMessage("End Failed", 1, false, true); });

  ArduinoOTA.begin();
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
  screen->DisplayText();
  server.handleClient();
  ArduinoOTA.handle();
}
