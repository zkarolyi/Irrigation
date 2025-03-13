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

float temperature, humidity, pressure, altitude;

const char *ssid;
const char *password;

WebServer server(80);
bool wifiConnected = false;
IrrigationSchedules schedules(relayPins);
Display *screen;
Menu *menu;
bool isMenuActive = false;

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

bool InitializeWiFi()
{
  screen->DisplayMessage("Connecting to ", 0);
  screen->DisplayMessage(ssid, 0, false, true);
  Serial.println("Connect to router");

  // connect to your local wi-fi network
  WiFi.disconnect();
  WiFi.hostname(HOSTNAME);
  WiFi.begin(ssid, password);

  screen->DisplayMessage(".", 1, true, false);
  // check wi-fi is connected to wi-fi network
  int countDown = 10;
  while (WiFi.status() != WL_CONNECTED && countDown-- > 0)
  {
    delay(1000);
    screen->DisplayMessage(".", 1, false, false);
  }
  if (countDown > 0)
  {
    screen->DisplayMessage("WI-FI connected", 0, true, true);
    screen->DisplayMessage("IP address: ", 1);
    screen->DisplayMessage(WiFi.localIP().toString(), 1, false, true);

    wifiIpAddress = WiFi.localIP().toString();
    wifiDnsIp = WiFi.dnsIP().toString();
    wifiGatewayIp = WiFi.gatewayIP().toString();
    wifiHostname = WiFi.getHostname();
    wifiMacAddress = WiFi.macAddress();
    wifiSsid = WiFi.SSID();
    return true;
  }
  screen->DisplayMessage("WI-FI connection failed", 1, true, true);
  return false;
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
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
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
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
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
  menu->GenerateIrrigationSubmenu(schedules.getNumberOfSchedules());
}

void handle_onScheduleList()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
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
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Handle get settings", 0, true, true);
  String jsonString = convertToJson(schedules);
  server.send(200, "text/html", jsonString);
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnRoot()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
  screen->DisplayMessage("Starting handle request", 0, true, true);

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
        body.replace("{{SetActive}}", "document.getElementById('ch" + String(i+1) + "').classList.add('button-check');");
      }
      file.close();
    }

  } else {
    body = "Failed to open file";
  }

  server.send(200, "text/html", body);
  screen->DisplayMessage("Finished.", 1, true, true);
}

void handle_OnToggleSwitch()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
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
  displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
  server.send(200, "application/json",
              "{\"status\": \"OK\", \"channel\": " + String(ch + 1) + ", \"pin\": " + String(pin) + "}");
  screen->DisplayMessage("Finished on: " + String(ch), 1, true, true);
}

void handle_NotFound()
{
  displayNetworkActivity = DISPLAY_TIMEOUT_INTERVAL;
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
  if (!irrigationScheduleEnabled || !wifiConnected || millis() - irrigationLastCheck < irrigationCheckInterval)
  {
    return;
  }
  irrigationLastCheck = millis();

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  int currentTime = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  int channelToStart = -1;
  for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
  {
    IrrigationSchedule schedule = schedules.getSchedule(i);
    int startTime = schedule.getStartTime() * 60;
    for (int j = 0; j < schedule.getNumberOfChannels(); j++)
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
        screen->DisplayMessage("Channel " + String(i + 1) + " stopped", 1, true, true);
      }
    }
    else
    {
      if (digitalRead(schedules.getPin(i)) == HIGH)
      {
        digitalWrite(schedules.getPin(i), LOW);
        displayOutChange = DISPLAY_TIMEOUT_INTERVAL;
        screen->DisplayMessage("Channel " + String(i + 1) + " started", 2, true, true);
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

  // InitializeLCD();
  InicializeRelays();
  screen = new Display(relayPins, displayDimmPin);

  InitFS();

  if (readWiFiCredentials())
  {
    wifiConnected = InitializeWiFi();
  }
  else
  {
    screen->DisplayMessage("No wifi credentials", 1, true, true);
  }

  InitializeSchedules();

  if (wifiConnected)
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
    screen->DisplayMessage("HTTP server started", 0, true, true);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  else
  {
    screen->DisplayMessage("HTTP server not started", 0, true, true);
  }

  // lcd.write(0);

  Serial.println("Starting menu");
  menu = new Menu();
  menu->renderer.begin();
  menu->GenerateIrrigationSubmenu(schedules.getNumberOfSchedules());
  menu->menu.setScreen(mainScreen);
  menu->menu.hide();
  Serial.println("Menu started");

  if (wifiConnected)
  {
    InitializeOTA();
  }
  else
  {
    screen->DisplayMessage("OTA not started", 0, true, true);
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
}

// Callback functions

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
  screen->DisplayMessage("SSID: " + wifiSsid, 0, true, true);
  screen->DisplayMessage("IP: " + wifiIpAddress, 1, true, true);
  screen->DisplayMessage("Hostname: " + wifiHostname, 2, true, true);
}

void toggleChannel(int channel)
{
  Serial.printf("Channel %d toggle start\n", channel);
  screen->DisplayMessage("Handle toggle", 0, true, true);
  if (channel < -1 || channel > 7)
  {
    screen->DisplayMessage("Invalid channel", 1, true, true);
    Serial.println("Invalid channel");
    return;
  }

  if (channel == -1)
  {
    irrigationScheduleEnabled = true;
    screen->DisplayMessage("Finished on: all", 1, true, true);
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
    screen->DisplayMessage("Finished on: " + String(channel), 1, true, true);
    Serial.printf("Channel %d toggled\n", channel);
  }

  exitMenuCallback();
}