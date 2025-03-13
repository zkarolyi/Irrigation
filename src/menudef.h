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

// Manual control submenu
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

// Schedules submenu
MenuScreen *schedulesScreen; // dinamically generated

// Settings submenu
MENU_SCREEN(settingsScreen, settingsItems,
            ITEM_WIDGET(
                "Backlight",
                backlightCallback,
                WIDGET_RANGE(100, 10, 50, 250, "%dm", 1)),
            ITEM_COMMAND("Wifi Information", wifiInformationCallback),
            ITEM_SUBMENU("Wifi Settings", wifiSettingsScreen));

// Wifi settings submenu
MENU_SCREEN(wifiSettingsScreen, wifiSettingsItems,
            ITEM_INPUT_CHARSET("SSID", ssidCharset, inputSsidCallback),
            ITEM_INPUT_CHARSET("Password", passwordCharset, inputPwdCallback),
            ITEM_COMMAND("Connect", commandWifiCallback));
