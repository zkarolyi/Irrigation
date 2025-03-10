#pragma once

#include "menu.h"

const char *ssidCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char *passwordCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";

MENU_SCREEN(mainScreen, mainItems,
            ITEM_SUBMENU("Manual Control", manualScreen),
            ITEM_SUBMENU("Schedules", schedulesScreen),
            ITEM_SUBMENU("Settings", settingsScreen),
            ITEM_COMMAND("Exit Menu", exitMenuCallback));

// Create submenu and precise its parent
MENU_SCREEN(manualScreen, manualItems,
            ITEM_COMMAND_INT("CH 1", 0, toggleChannel),
            ITEM_COMMAND_INT("CH 2", 1, toggleChannel),
            ITEM_COMMAND_INT("CH 3", 2, toggleChannel),
            ITEM_COMMAND_INT("CH 4", 3, toggleChannel),
            ITEM_COMMAND_INT("CH 5", 4, toggleChannel),
            ITEM_COMMAND_INT("CH 6", 5, toggleChannel),
            ITEM_COMMAND_INT("CH 7", 6, toggleChannel),
            ITEM_COMMAND_INT("CH 8", 7, toggleChannel),
            ITEM_COMMAND_INT("All off", -1, toggleChannel));

MenuScreen *schedulesScreen; // dinamically generated

MENU_SCREEN(settingsScreen, settingsItems,
            ITEM_WIDGET(
                "Backlight",
                backlightCallback,
                WIDGET_RANGE(100, 10, 50, 250, "%dm", 1)),
            ITEM_SUBMENU("Wifi Information", wifiInformationScreen),
            ITEM_SUBMENU("Wifi Settings", wifiSettingsScreen));

MENU_SCREEN(wifiInformationScreen, wifiInformationItems,
            ITEM_VALUE("SSID", wifiSsid),
            ITEM_VALUE("Hn ", wifiHostname, "%s"),
            ITEM_VALUE("IP ", wifiIpAddress),
            ITEM_VALUE("Gw ", wifiGatewayIp),
            ITEM_VALUE("DNS ", wifiDnsIp),
            ITEM_BACK());

MENU_SCREEN(wifiSettingsScreen, wifiSettingsItems,
            ITEM_INPUT_CHARSET("SSID", ssidCharset, inputSsidCallback),
            ITEM_INPUT_CHARSET("Password", passwordCharset, inputPwdCallback),
            ITEM_COMMAND("Connect", commandWifiCallback));
