#pragma once

#include "menu.h"

const char *ssidCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char *passwordCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";

// Main menu
MENU_SCREEN(mainScreen, mainItems,
            ITEM_SUBMENU("Manual Control", manualScreen),
            ITEM_SUBMENU("Schedules", schedulesScreen),
            ITEM_SUBMENU("Settings", settingsScreen),
            ITEM_COMMAND("Exit Menu", exitMenuCallback));

MenuScreen *manualScreen; // dinamically generated

MenuScreen *schedulesScreen; // dinamically generated

MenuScreen *scheduleViewScreen; // dinamically generated

MenuScreen *scheduleEditScreen; // dinamically generated

// Settings submenu
MENU_SCREEN(settingsScreen, settingsItems,
            ITEM_COMMAND("Wifi Information", wifiInformationCallback),
            ITEM_SUBMENU("Wifi Settings", wifiSettingsScreen),
            ITEM_COMMAND("Reset Screen", resetScreenCallback),
            ITEM_COMMAND("Reset RTC", resetRtcCallback),
            ITEM_BACK("Back"));

// Wifi settings submenu
MENU_SCREEN(wifiSettingsScreen, wifiSettingsItems,
            ITEM_INPUT_CHARSET("SSID", ssidCharset, inputSsidCallback),
            ITEM_INPUT_CHARSET("Password", passwordCharset, inputPwdCallback),
            ITEM_COMMAND("Connect", commandWifiCallback),
            ITEM_BACK("Cancel"));
