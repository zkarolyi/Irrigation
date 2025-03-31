#ifndef MENU_H
#define MENU_H

#include "globals.h"
#include <LiquidCrystal_I2C.h>
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <ItemWidget.h>
#include <ItemSubMenu.h>
#include <ItemToggle.h>
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
    LcdMenu menu;
    SimpleRotaryAdapter rotaryInput;
    CharacterDisplayRenderer renderer;
    SimpleRotary encoder;
    void GenerateManualScreen();
    void GenerateIrrigationSubmenu();
    void GenerateScheduleViewSubmenu(int scheduleIndex);
    void GenerateScheduleEditSubmenu(int scheduleIndex);
  private:
    LiquidCrystal_I2CAdapter lcdAdapter;
    void DeleteScreen(MenuScreen *screen);
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
extern void commandAddScheduleCallback();
extern void commandScheduleSelectCallback(int scheduleIndex);
extern void commandScheduleEditCallback(int scheduleIndex);
extern void commandScheduleDeleteCallback(int scheduleIndex);
extern void commandScheduleSaveCallback(int scheduleIndex);

// Screens
extern MenuScreen* mainScreen;
extern MenuScreen* manualScreen;
extern MenuScreen* schedulesScreen;
extern MenuScreen* scheduleViewScreen;
extern MenuScreen* scheduleEditScreen;
extern MenuScreen* settingsScreen;
extern MenuScreen* wifiSettingsScreen;

#endif // MENU_H