#pragma once

#include "menu.h"

const char *ssidCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.";
const char *passwordCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";

// Main menu
MENU_SCREEN(mainScreen, mainItems,
            ITEM_SUBMENU("Manual Control", manualScreen),
            ITEM_SUBMENU("Schedules", schedulesScreen),
            ITEM_SUBMENU("Settings", settingsScreen),
            ITEM_COMMAND("Exit Menu", exitMenuCallback));

MenuScreen *manualScreen; // dinamically generated

MenuScreen *schedulesScreen; // dinamically generated

MenuScreen *scheduleDaysScreen; // dinamically generated

MenuScreen *scheduleViewScreen; // dinamically generated

MenuScreen *scheduleEditScreen; // dinamically generated

// Settings submenu
MENU_SCREEN(settingsScreen, settingsItems,
            ITEM_COMMAND("Wifi Information", wifiInformationCallback),
            ITEM_SUBMENU("Wifi Settings", wifiSettingsScreen),
            ITEM_SUBMENU("MQTT Settings", mqttSettingsScreen),
            ITEM_COMMAND("Reset Screen", resetScreenCallback),
            ITEM_COMMAND("Reset RTC", resetRtcCallback),
            ITEM_BACK("Back"));

// Wifi settings submenu
MENU_SCREEN(wifiSettingsScreen, wifiSettingsItems,
            ITEM_INPUT_CHARSET("SSID", ssidCharset, nullptr),
            ITEM_INPUT_CHARSET("Password", passwordCharset, nullptr),
            ITEM_COMMAND("Connect", commandWifiCallback),
            ITEM_BACK("Cancel"));

// Mqtt settings submenu
MENU_SCREEN(mqttSettingsScreen, mqttSettingsItems,
            ITEM_INPUT_CHARSET("Broker", ssidCharset, nullptr),
            ITEM_WIDGET(
                "Port",
                [](int port)
                { Serial.println(port); },
                WIDGET_RANGE(1883, 1, 1, 65535, "%d", 0, true)),
            ITEM_INPUT_CHARSET("Username", ssidCharset, nullptr),
            ITEM_INPUT_CHARSET("Password", passwordCharset, nullptr),
            ITEM_COMMAND("Connect", commandMqttCallback),
            ITEM_BACK("Cancel"));