#include <vector>
#include <string>
#include "settings.h"
#include "menu.h"
#include "menudef.h"

void inputSsidCallback(char *value)
{
    Serial.print(F("# "));
    Serial.println(value);
}

void inputPwdCallback(char *value)
{
    Serial.print(F("# "));
    Serial.println(value);
}

void commandWifiCallback()
{
    Serial.println("Wifi connect");
    char *ssid = (static_cast<ItemInputCharset *>(wifiSettingsItems[0]))->getValue(); // wifiSettingsScreen->getItemAt(0)->getText();
    char *pwd = (static_cast<ItemInputCharset *>(wifiSettingsItems[1]))->getValue();  // wifiSettingsScreen->getItem(1)->getValue();
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(pwd);
    saveWiFiCredentials(ssid, pwd);
    exitMenuCallback();
}

void toggleCallback(bool isOn, int index)
{
    Serial.printf("Channel %d %s\n", index, isOn ? "started" : "stopped");
}

void commandCallback()
{
    Serial.println("Callback");
}

void commandAddScheduleCallback()
{
    Serial.println("Add schedule");
}

Menu::Menu()
    : lcdAdapter(new LiquidCrystal_I2C(DISPLAY_ADDRESS, DISPLAY_COLUMNS, DISPLAY_LINES)),
      renderer(CharacterDisplayRenderer(&lcdAdapter, DISPLAY_COLUMNS, DISPLAY_LINES, 0x7E, 0x7F, NULL, NULL)),
      menu(LcdMenu(renderer)),
      encoder(ROTARY_ENCODER_PIN1, ROTARY_ENCODER_PIN2, ROTARY_ENCODER_BUTTON),
      rotaryInput(&menu, &encoder)
{
    Serial.println("Menu initialized");
}

void Menu::GenerateManualScreen(){
    // Clean up old manualScreen and its items if they exist
    if (manualScreen != nullptr)
    {
        Serial.println("Deleting old manualScreen");
        int i = 0;
        MenuItem *item;
        while ((item = manualScreen->getItemAt(i)) != nullptr)
        {
            Serial.printf("Deleting item %d\n", i);
            delete item;
            i++;
        }
        delete manualScreen;
    }

    int numberOfChannels = schedules.getNumberOfChannels();
    int ch = 0;
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[numberOfChannels + 3];
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch " + String(i + 1);
        items[ch++] = new ItemCommandInt(strdup(itemName.c_str()), i, toggleChannel);
    }
    items[ch++] = new ItemCommandInt("All off", -1, toggleChannel);
    items[ch++] = new ItemBack("Back");
    items[ch++] = nullptr;
    manualScreen = new MenuScreen(items);
}

void Menu::GenerateIrrigationSubmenu()
{
    // Clean up old schedulesScreen and its items if they exist
    if (schedulesScreen != nullptr)
    {
        Serial.println("Deleting old schedulesScreen");
        int i = 0;
        MenuItem *item;
        while ((item = schedulesScreen->getItemAt(i)) != nullptr)
        {
            Serial.printf("Deleting item %d\n", i);
            delete item;
            i++;
        }
        delete schedulesScreen;
    }

    int numberOfSchedules = schedules.getNumberOfSchedules();

    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[numberOfSchedules + 3];
    for (int i = 0; i < numberOfSchedules; i++)
    {
        String itemName = String(i + 1) + "," + schedules.getSchedule(i).getStartTimeString() + "-" + IrrigationDaysToRunToString(schedules.getSchedule(i).getDaysToRun());
        // items[i] = new MenuItem(strdup(itemName.c_str()));
        items[i] = new ItemCommandInt(strdup(itemName.c_str()), i, commandScheduleSelectCallback);
    }

    items[numberOfSchedules] = new ItemCommand("Add schedule", commandAddScheduleCallback);
    items[numberOfSchedules + 1] = new ItemBack("Back");
    items[numberOfSchedules + 2] = nullptr;
    schedulesScreen = new MenuScreen(items);
}

void Menu::GenerateScheduleViewSubmenu(int scheduleIndex)
{
    Serial.printf("Generating schedule view for schedule %d\n", scheduleIndex);
    // Clean up old scheduleViewScreen and its items if they exist
    if (scheduleViewScreen != nullptr)
    {
        Serial.println("Deleting old scheduleViewScreen");
        int i = 0;
        MenuItem *item;
        while ((item = scheduleViewScreen->getItemAt(i)) != nullptr)
        {
            Serial.printf("Deleting item %d\n", i);
            delete item;
            i++;
        }
        delete scheduleViewScreen;
    }

    IrrigationSchedule schedule = schedules.getSchedule(scheduleIndex);
    int numberOfChannels = schedules.getNumberOfChannels();
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[6 + numberOfChannels];
    int ch = 0;
    items[ch++] = new MenuItem(strdup(schedule.getStartTimeString().c_str()));
    items[ch++] = new MenuItem(strdup(IrrigationDaysToRunToString(schedule.getDaysToRun()).c_str()));
    items[ch++] = new MenuItem(strdup((String(schedule.getWeight()) + "%" ).c_str()));
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = String(i + 1) + ": " + String(schedule.getChannelDuration(i)) + " min";
        Serial.println(itemName);
        items[ch++] = new MenuItem(strdup(itemName.c_str()));
    }
    items[ch++] = new ItemCommandInt("Edit", scheduleIndex , commandScheduleEditCallback);
    items[ch++] = new ItemBack("Back");
    items[ch++] = nullptr;
    Serial.println("Creating scheduleViewScreen");
    scheduleViewScreen = new MenuScreen(items);
    scheduleViewScreen->setParent(schedulesScreen);
}
