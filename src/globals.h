#ifndef STATUSES_H
#define STATUSES_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "menu.h"
#include "Irrigation.h"
#include <RTClib.h>
#include <PubSubClient.h>

extern bool isMenuActive;

extern std::vector<int> relayPins;

extern RTC_DS3231 rtc;

extern IrrigationSchedules schedules;
extern bool irrigationScheduleEnabled;
extern int irrigationManualEnd;
extern int displayNetworkActivity;
extern int displayOutChange;
extern void SaveSchedules(IrrigationSchedules &schedules);

extern int rotaryEncoderPosition;
extern int rotaryEncoderButtonDuration;

extern void saveWiFiCredentials(const char *ssid, const char *password);
extern String wifiIpAddress;
extern String wifiDnsIp;
extern String wifiGatewayIp;
extern String wifiHostname;
extern String wifiMacAddress;
extern String wifiSsid;

extern void saveMqttCredentials(const char *broker, int port, const char *username, const char *password);

extern LiquidCrystal_I2C lcd;

#endif