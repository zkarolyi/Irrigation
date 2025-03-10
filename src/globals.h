#ifndef STATUSES_H
#define STATUSES_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

extern bool isMenuActive;

extern bool irrigationScheduleEnabled;
extern int displayNetworkActivity;
extern int displayOutChange;

extern int rotaryEncoderPosition;
extern int rotaryEncoderButtonDuration;

extern void saveWiFiCredentials(const char *ssid, const char *password);
extern bool WifiConnected;
extern String wifiIpAddress;
extern String wifiDnsIp;
extern String wifiGatewayIp;
extern const char* wifiHostname;
extern String wifiMacAddress;
extern String wifiSsid;  

extern LiquidCrystal_I2C lcd;

#endif