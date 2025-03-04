#ifndef STATUSES_H
#define STATUSES_H

extern bool isMenuActive;

extern bool irrigationScheduleEnabled;
extern int displayNetworkActivity;
extern int displayOutChange;

extern int rotaryEncoderPosition;
extern int rotaryEncoderButtonDuration;

extern String wifiIpAddress;
extern String wifiDnsIp;
extern String wifiGatewayIp;
extern String wifiHostname;
extern String wifiMacAddress;
extern String wifiSsid;  

extern LiquidCrystal_I2C lcd;

#endif