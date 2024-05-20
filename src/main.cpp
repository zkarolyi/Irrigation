#include <WiFi.h>
#include <WebServer.h>
#include <main.h>
#include "SPIFFS.h"
#include <LiquidCrystal_I2C.h>
#include <ArduinoOTA.h>
#include <Arduino.h>
#include "Irrigation.h"
#include <ArduinoJson.h>

using namespace std;

float temperature, humidity, pressure, altitude;
bool ch[irrigationChannels] = {false, false, false, false, false, false, false, false};

/*Put your SSID & Password*/
const char *ssid = "Csepp2";       // Enter SSID here
const char *password = "Karolyi1"; // Enter Password here

vector<String> v;

WebServer server(80);
IrrigationSchedules schedules;

LiquidCrystal_I2C lcd(0x27, displayColumns, displayLines); // set the LCD address to 0x27 (0x3F) for a 16 chars and 2 line display
static unsigned long lastScrollTime = 0;

#include <cmath>

void DisplayBigNumber(int row, int column, int num, bool colon = false)
{
  for (int i = 3; i >= 0; i--)
  {
    int digit = static_cast<int>(num / pow(10, i)) % 10;

    lcd.setCursor(column, row);
    lcd.write(bigNumbers[digit][0]);
    lcd.write(bigNumbers[digit][1]);
    lcd.write(bigNumbers[digit][2]);
    if (i == 2 && colon)
    {
      lcd.write(B10100101);
    }
    else
    {
      lcd.write(254);
    }

    lcd.setCursor(column, row + 1);
    lcd.write(bigNumbers[digit][3]);
    lcd.write(bigNumbers[digit][4]);
    lcd.write(bigNumbers[digit][5]);
    if (i == 2 && colon)
    {
      lcd.write(B10100101);
    }
    else
    {
      lcd.write(254);
    }

    column += 4;
  }
}

void DisplayText()
{
  if (millis() - displayLastUpdate < displayLastUpdateInterval)
  {
    return;
  }

  displayLastUpdate = millis();

  if (displayTimeout > 0)
  {
    displayTimeout -= displayLastUpdateInterval;
    // Timeout, clear the display
    if (displayTimeout > 0)
    {
      for (int i = 0; i < displayLines; i++)
      {
        lcd.setCursor(0, i);
        if (displayLinesText[i].length() <= displayColumns)
        {
          lcd.print(displayLinesText[i]);
          for (int j = displayLinesText[i].length(); j < displayColumns; j++)
          {
            lcd.print(" ");
          }
        }
        else
        {
          if (displayLinesPosition[i] + displayColumns > displayLinesText[i].length())
          {
            lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesText[i].length()));
            lcd.print(displayLinesText[i].substring(0, displayColumns - (displayLinesText[i].length() - displayLinesPosition[i])));
          }
          else
          {
            lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesPosition[i] + displayColumns));
          }
          displayLinesPosition[i]++;
          if (displayLinesPosition[i] >= displayLinesText[i].length())
          {
            displayLinesPosition[i] = 0;
          }
        }
      }
    }
    else
    {
      for (size_t i = 0; i < displayLines; i++)
      {
        displayLinesText[i] = "";
      }

      lcd.clear();
    }
  }
  else
  {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      displayLinesText[0] = "--/--/--";
    }
    DisplayBigNumber(1, 2, timeinfo.tm_hour * 100 + timeinfo.tm_min, timeinfo.tm_sec % 2 == 0);
  }
}

void Display(String message, int row, bool first = true, bool last = false)
{
  displayTimeout = displayTimeoutInterval;
  Serial.println(message);
  if (first)
  {
    displayLinesText[row] = "";
  }
  displayLinesText[row] += message;
  if (last)
  {
    if (displayLinesText[row].length() > displayColumns)
    {
      displayLinesText[row] += "   ";
    }
    while (displayLinesText[row].length() < displayColumns)
    {
      displayLinesText[row] += " ";
    }
  }
  displayLinesPosition[row] = 0;
  DisplayText();
}

void InitializeLCD()
{
  lcd.init();
  lcd.clear();
  lcd.backlight();
  pinMode(displayDimmPin, OUTPUT);
  analogWrite(displayDimmPin, 255);

  lcd.createChar(0, LT);
  lcd.createChar(1, UB);
  lcd.createChar(2, RT);
  lcd.createChar(3, LL);
  lcd.createChar(4, LB);
  lcd.createChar(5, LR);
  lcd.createChar(6, MB);
  // lcd.createChar(0, check);
  // lcd.createChar(1, cross);
  // lcd.createChar(2, retarrow);
}

void InicializeRelays()
{
  for (int i = 0; i < irrigationChannels; i++)
  {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }
}

// https://arduinojson.org/
void InitializeSchedules()
{
  File file = SPIFFS.open(schedulesFile);
  if (!file)
  {
    Display("Failed to open schedules", 2, true, true);
    return;
  }

  String json = file.readString();
  file.close();

  convertFromJson(json, schedules);
}

void SaveSchedules(IrrigationSchedules &schedules)
{
  File file = SPIFFS.open(schedulesFile, "w");
  if (!file)
  {
    Display("Failed to open file for writing", 2, true, true);
    return;
  }

  String json = convertToJson(schedules);
  if (file.write((uint8_t *)json.c_str(), json.length()) != json.length())
  {
    Display("Failed to write schedules", 2, true, true);
  }

  file.close();
}

void handle_OnSetDimming()
{
  Display("Handle set dimming", 0, true, true);
  int dimm = server.hasArg("dimm") ? server.arg("dimm").toInt() : 255;
  if (dimm < 0 || dimm > 255)
  {
    Display("Invalid dimming", 1, true, true);
    server.send(400, "text/html", "Invalid dimming");
    return;
  }
  analogWrite(displayDimmPin, dimm);
  server.send(200, "text/html", "OK (" + String(dimm) + ")");
  Display("Finished on: " + String(dimm), 1, true, true);
}

void handle_OnGetSchedule()
{
  Display("Handle get schedule", 0, true, true);
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
    for (int i = (int)IrrigationDaysToRun::Sunday; i <= (int)IrrigationDaysToRun::Every9days; i++)
    {
      body.replace("${dTR" + String(i) + "}", dtr == (IrrigationDaysToRun)i ? " selected" : "");
    }
    int weight = 100;
    for (int i = 50; i <= 150; i = i + 25)
    {
      body.replace("${W" + String(i) + "}", weight == i ? " selected" : "");
    }
    for (int i = 0; i < irrigationChannels; i++)
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
    Display("R:" + String(dtr), 2, true, false);
    for (int i = (int)IrrigationDaysToRun::Sunday; i <= (int)IrrigationDaysToRun::Every9days; i++)
    {
      body.replace("${dTR" + String(i) + "}", dtr == i ? " selected" : "");
    }
    int weight = sch.getWeight();
    for (int i = 50; i <= 150; i = i + 25)
    {
      body.replace("${W" + String(i) + "}", weight == i ? " selected" : "");
    }
    for (int i = 0; i < irrigationChannels; i++)
    {
      int cd = sch.getChannelDuration(i);
      body.replace("${duration" + String(i + 1) + "}", String(cd));
    }
  }

  server.send(200, "text/html", body);
  Display("Finished.", 1, true, true);
}

void handle_OnSetSchedule()
{
  Display("Handle set schedule", 0, true, true);
  int id = server.hasArg("id") ? server.arg("id").toInt() : -1;
  int getStartTime = server.hasArg("startTimeHour") && server.hasArg("startTimeMinute") ? server.arg("startTimeHour").toInt() * 60 + server.arg("startTimeMinute").toInt() : -1;
  if (getStartTime < 0 || getStartTime > 1440)
  {
    Display("Invalid start time", 1, true, true);
    server.send(400, "text/html", "Invalid start time");
    return;
  }

  for (int i = 0; i < irrigationChannels; i++)
  {
    if (!server.hasArg("duration" + String(i + 1)) || server.arg("duration" + String(i + 1)).length() == 0 || server.arg("duration" + String(i + 1)).toInt() < 0 || server.arg("duration" + String(i + 1)).toInt() > 60)
    {
      Display("Invalid duration", 1, true, true);
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
  for (int i = 0; i < irrigationChannels; i++)
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
  Display("Finished.", 1, true, true);
}

void handle_onScheduleList()
{
  Display("Handle schedule list", 0, true, true);
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
  Display("Finished.", 1, true, true);
}

void handle_OnGetSettings()
{
  Display("Handle get settings", 0, true, true);
  String jsonString = convertToJson(schedules);
  server.send(200, "text/html", jsonString);
  Display("Finished.", 1, true, true);
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  InitializeLCD();
  InicializeRelays();

  Display("Connecting to ", 0);
  Display(ssid, 0, false, true);

  // connect to your local wi-fi network
  WiFi.begin(ssid, password);

  Display(".", 1, true, false);
  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Display(".", 1, false, false);
  }
  Display("WI-FI connected", 0, true, true);
  Display("IP address: ", 1);
  Display(WiFi.localIP().toString(), 1, false, true);

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
  Display("HTTP server started", 0, true, true);
  lcd.write(0);

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
      Display("Updating " + type, 0, true, true); })
      .onEnd([]()
             { Display("End", 0, true, true); })
      .onProgress([](unsigned int progress, unsigned int total)
                  {
                    Display("Progress: " + String((progress / (total / 100))) + "%", 1, true, true);
                    // esp_task_wdt_reset();
                  })
      .onError([](ota_error_t error)
               {
                Display("Error[" + String(error) + "]: ", 0, true, true);
                if (error == OTA_AUTH_ERROR) Display("Auth Failed", 1, false, true);
                else if (error == OTA_BEGIN_ERROR) Display("Begin Failed", 1, false, true);
                else if (error == OTA_CONNECT_ERROR) Display("Connect Failed", 1, false, true);
                else if (error == OTA_RECEIVE_ERROR) Display("Receive Failed", 1, false, true);
                else if (error == OTA_END_ERROR) Display("End Failed", 1, false, true); });

  ArduinoOTA.begin();
}

void loop()
{
  if (Serial.available())
  {
    char c = Serial.read();
    int cInt = c - '0';
    // digitalWrite(relayPins[cInt], !digitalRead(relayPins[cInt]));
    analogWrite(5, cInt * 50);
  }

  DisplayText();
  server.handleClient();
  ArduinoOTA.handle();
}

void handle_OnRoot()
{
  Display("Starting handle request", 0, true, true);
  for (int i = 0; i < irrigationChannels; i++)
  {
    ch[i] = digitalRead(relayPins[i]);
  }
  String body = GetHtml(ch);
  server.send(200, "text/html", body);
  Display("Finished.", 1, true, true);
}

void handle_OnToggleSwitch()
{
  Display("Handle toggle", 0, true, true);
  int ch = server.hasArg("ch") ? server.arg("ch").toInt() - 1 : -1;
  if (ch < 0 || ch > 7)
  {
    Display("Invalid channel", 1, true, true);
    server.send(400, "text/html", "Invalid channel");
    return;
  }
  if (digitalRead(relayPins[ch]) == HIGH)
  {
    for (int i = 0; i < irrigationChannels; i++)
    {
      digitalWrite(relayPins[i], HIGH);
    }
    digitalWrite(relayPins[ch], LOW);
  }
  else
  {
    digitalWrite(relayPins[ch], HIGH);
  }
  server.send(200, "text/html", "OK (" + String(ch) + ")");
  Display("Finished on: " + String(ch), 1, true, true);
}

void handle_NotFound()
{
  Display("Page not found", 0, true, true);
  server.send(404, "text/plain", "Page not found");
}

String GetHtml(bool ch[])
{
  String ptr;
  for (String s : v)
  {
    ptr += s;
  }

  for (int i = 0; i < irrigationChannels; i++)
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

void InitFS()
{
  if (!SPIFFS.begin(true))
  {
    Display("An Error has occurred while mounting SPIFFS", 0, true, true);
    return;
  }
}

void GetFile(String fileName)
{
  File file = SPIFFS.open(fileName);
  if (!file)
  {
    Display("Failed to open file for reading", 0, true, true);
    return;
  }

  // vector<String> v;
  while (file.available())
  {
    v.push_back(file.readStringUntil('\n'));
  }

  file.close();
}
