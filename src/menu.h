#ifndef MENU_H
#define MENU_H

#include "globals.h"
#include <LiquidCrystal_I2C.h>
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <ItemList.h>
#include <ItemWidget.h>
#include <ItemSubMenu.h>
#include <ItemToggle.h>
#include <ItemToggleInt.h>
#include <ItemCommandInt.h>
#include <ItemCommand.h>
#include <ItemInputCharset.h>
#include <ItemValue.h>
#include <ItemBack.h>
#include <SimpleRotary.h>
#include <input/SimpleRotaryAdapter.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
#include <widget/WidgetList.h>
#include <widget/WidgetRange.h>

class Menu
{
  public:
    Menu();
    void GenerateIrrigationSubmenu(int numberOfSchedules);
    LcdMenu menu;
    SimpleRotaryAdapter rotaryInput;
    CharacterDisplayRenderer renderer;
    SimpleRotary encoder;
  private:
    LiquidCrystal_I2CAdapter lcdAdapter;
};

// Callback functions
extern void toggleChannel(int channel);
extern void toggleCallback(bool isOn, int index);
extern void commandCallback();
extern void exitMenuCallback();
extern void inputSsidCallback(char *value);
extern void inputPwdCallback(char *value);
extern void commandWifiCallback();
extern void wifiInformationCallback();
extern void backlightCallback(int value);

// Screens
extern MenuScreen* mainScreen;
extern MenuScreen* manualScreen;
extern MenuScreen* schedulesScreen;
extern MenuScreen* settingsScreen;
extern MenuScreen* wifiSettingsScreen;

#endif // MENU_H