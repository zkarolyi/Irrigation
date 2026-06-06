#ifndef STATUSES_H
#define STATUSES_H

#include <Arduino.h>
#include "Irrigation.h"
#include <RTClib.h>
#include <PubSubClient.h>

extern RTC_DS3231 rtc;

extern IrrigationSchedules schedules;
extern bool irrigationScheduleEnabled;
extern unsigned long irrigationManualEnd;
extern int displayNetworkActivity;
extern int displayOutChange;
extern void saveWiFiCredentials(const char *ssid, const char *password);
extern String wifiIpAddress;
extern String wifiDnsIp;
extern String wifiGatewayIp;
extern String wifiHostname;
extern String wifiMacAddress;
extern String wifiSsid;

extern void saveMqttCredentials(const char *broker, int port, const char *username, const char *password);

#endif