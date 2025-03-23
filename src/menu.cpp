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

void Menu::GenerateManualScreen()
{
    // Clean up old manualScreen and its items if they exist
    DeleteScreen(manualScreen);

    int numberOfChannels = schedules.getNumberOfChannels();
    int ch = 0;
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[numberOfChannels + 3];
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1) + " toggle";
        items[ch++] = new ItemCommandInt(strdup(itemName.c_str()), i, toggleChannel);
    }
    items[ch++] = new ItemCommandInt("Off, schedule", -1, toggleChannel);
    items[ch++] = new ItemBack("Back");
    items[ch++] = nullptr;
    manualScreen = new MenuScreen(items);
}

void Menu::GenerateIrrigationSubmenu()
{
    // Clean up old schedulesScreen and its items if they exist
    DeleteScreen(schedulesScreen);

    int numberOfSchedules = schedules.getNumberOfSchedules();

    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[numberOfSchedules + 3];
    for (int i = 0; i < numberOfSchedules; i++)
    {
        String itemName = String(i + 1) + "." + schedules.getSchedule(i).getStartTimeString() + "-" + daysToRunValues[schedules.getSchedule(i).getDaysToRun()];
        // items[i] = new MenuItem(strdup(itemName.c_str()));
        items[i] = new ItemCommandInt(strdup(itemName.c_str()), i, commandScheduleSelectCallback);
    }

    items[numberOfSchedules] = new ItemCommandInt("Add schedule", -1, commandScheduleEditCallback);
    items[numberOfSchedules + 1] = new ItemBack("Back");
    items[numberOfSchedules + 2] = nullptr;
    schedulesScreen = new MenuScreen(items);
}

void Menu::GenerateScheduleViewSubmenu(int scheduleIndex)
{
    // Clean up old scheduleViewScreen and its items if they exist
    DeleteScreen(scheduleViewScreen);

    IrrigationSchedule schedule = schedules.getSchedule(scheduleIndex);
    int numberOfChannels = schedules.getNumberOfChannels();
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[6 + numberOfChannels];
    int ch = 0;
    items[ch++] = new MenuItem(strdup(("Start: " + schedule.getStartTimeString()).c_str()));
    items[ch++] = new MenuItem(strdup(("Days: " + String(daysToRunValues[schedule.getDaysToRun()])).c_str()));
    items[ch++] = new MenuItem(strdup(("Weight: " + String(schedule.getWeight()) + "%").c_str()));
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1) + ": " + String(schedule.getChannelDuration(i)) + " min";
        items[ch++] = new MenuItem(strdup(itemName.c_str()));
    }
    items[ch++] = new ItemCommandInt("Edit", scheduleIndex, commandScheduleEditCallback);
    items[ch++] = new ItemBack("Back");
    items[ch++] = nullptr;
    scheduleViewScreen = new MenuScreen(items);
    scheduleViewScreen->setParent(schedulesScreen);
}

void Menu::GenerateScheduleEditSubmenu(int scheduleIndex)
{
    // Clean up old scheduleEditScreen and its items if they exist
    DeleteScreen(scheduleEditScreen);

    IrrigationSchedule schedule;
    if (scheduleIndex < 0)
    {
        schedule = IrrigationSchedule();
    }
    else
    {
        schedule = schedules.getSchedule(scheduleIndex);
    }

    int numberOfChannels = schedules.getNumberOfChannels();
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[7 + numberOfChannels];
    int ch = 0;
    items[ch++] = new ItemWidget<int, int>(
        "Start",
        new WidgetRange<int>(schedule.getStartTimeHours(), 1, 0, 23, " %02d", 0, false),
        new WidgetRange<int>(schedule.getStartTimeMinutes(), 15, 0, 45, ":%02d", 0, false),
        nullptr);
    // [](int hour, int minute)
    // { Serial.println(hour); Serial.println(minute); });
    items[ch++] = new ItemWidget<uint8_t>(
        "Days",
        new WidgetList<const char *>(daysToRunValues, daysToRunValuesSize, (int)schedule.getDaysToRun(), " %s", 0, true, nullptr),
        nullptr);
    // [](uint8_t day)
    // { Serial.println(day); });
    items[ch++] = new ItemWidget<int>(
        "Weight",
        new WidgetRange<int>(schedule.getWeight(), 25, 50, 150, " %d%%", 1, false),
        nullptr);
    // [](int weight)
    // { Serial.println(weight); });

    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1);
        items[ch++] = new ItemWidget<int>(
            strdup(itemName.c_str()),
            new WidgetRange<int>(schedule.getChannelDuration(i), 1, 0, 60, " %d min", 4, false),
            nullptr);
        // [](int duration)
        // { Serial.println(duration); });
    }
    items[ch++] = new ItemCommandInt("Save", scheduleIndex, commandScheduleSaveCallback);
    items[ch++] = new ItemCommandInt("Delete", scheduleIndex, commandScheduleDeleteCallback);
    items[ch++] = new ItemBack("Back");
    items[ch++] = nullptr;
    scheduleEditScreen = new MenuScreen(items);
    scheduleEditScreen->setParent(mainScreen);
}

void Menu::DeleteScreen(MenuScreen *screen)
{
    if (screen != nullptr)
    {
        int i = 0;
        MenuItem *item;
        while ((item = screen->getItemAt(i)) != nullptr)
        {
            delete item;
            i++;
        }
        delete screen;
    }
}