#include <vector>
#include <string>
#include "settings.h"
#include "menu.h"
#include "menudef.h"

void commandWifiCallback()
{
    Serial.println("Wifi connect");
    char *ssid = (static_cast<ItemInputCharset *>(wifiSettingsItems[0]))->getValue(); // wifiSettingsScreen->getItemAt(0)->getText();
    char *pwd = (static_cast<ItemInputCharset *>(wifiSettingsItems[1]))->getValue();  // wifiSettingsScreen->getItem(1)->getValue();
    saveWiFiCredentials(ssid, pwd);
    exitMenuCallback();
}

void commandMqttCallback()
{
    Serial.println("MQTT connect");
    char *broker = (static_cast<ItemInputCharset *>(mqttSettingsItems[0]))->getValue();
    int port = static_cast<WidgetRange<int> *>(static_cast<ItemWidget<int, int> *>(mqttSettingsItems[1])->getWidgetAt(0))->getValue();
    char *username = (static_cast<ItemInputCharset *>(mqttSettingsItems[2]))->getValue();
    char *password = (static_cast<ItemInputCharset *>(mqttSettingsItems[3]))->getValue();
    saveMqttCredentials(broker, port, username, password);
    exitMenuCallback();
}

void toggleCallback(bool isOn, int index)
{
    Serial.printf("Channel %d %s\n", index, isOn ? "started" : "stopped");
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
    Serial.println("Generating manual screen");
    if (manualScreen == nullptr)
    {
        manualScreen = new MenuScreen(std::vector<MenuItem *>{});
        Serial.println("Created new manual screen");
    }
    else
    {
        DeleteScreenItems(manualScreen);
    }
    manualScreen->addItem(
        new ItemWidget<int>(
            strdup("Duration"),
            new WidgetRange<int>(manualIrrigationDurationDef, 1, manualIrrigationDurationMin, manualIrrigationDurationMax, " %d", 0, true),
            nullptr));

    int numberOfChannels = schedules.getNumberOfChannels();
    int ch = 0;
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1) + " toggle";
        manualScreen->addItem(new ItemCommandInt(strdup(itemName.c_str()), i, toggleChannel));
    }
    manualScreen->addItem(new ItemCommandInt("Schedule On", -1, toggleChannel));
    manualScreen->addItem(new ItemBack("Back"));
    Serial.println("Manual screen generated with " + String(numberOfChannels) + " channels");
}

void Menu::GenerateIrrigationSubmenu()
{
    Serial.println("Generating irrigation submenu");
    if (schedulesScreen == nullptr)
    {
        schedulesScreen = new MenuScreen(std::vector<MenuItem *>{});
        Serial.println("Created new schedules screen");
    }
    else
    {
        DeleteScreenItems(schedulesScreen);
    }

    int numberOfSchedules = schedules.getNumberOfSchedules();

    DateTime now = rtc.now();

    for (int i = 0; i < numberOfSchedules; i++)
    {
        IrrigationSchedule sc = schedules.getSchedule(i);
        String itemName = String(i + 1) + "." + sc.getStartTimeString() + "-" + daysToRunValues[sc.getDaysToRun()];
        schedulesScreen->addItem(new ItemCommandInt(strdup(itemName.c_str()), i, commandScheduleSelectCallback));
    }
    schedulesScreen->addItem(new ItemCommandInt("Add schedule", -1, commandScheduleEditCallback));
    schedulesScreen->addItem(new ItemBack("Back"));
    Serial.println("Irrigation submenu generated with " + String(numberOfSchedules) + " schedules");
}

void Menu::GenerateScheduleViewSubmenu(int scheduleIndex)
{
    Serial.printf("Generating schedule view submenu for index %d\n", scheduleIndex);
    if (scheduleViewScreen == nullptr)
    {
        scheduleViewScreen = new MenuScreen(std::vector<MenuItem *>{});
        scheduleViewScreen->setParent(schedulesScreen);
    }
    else
    {
        DeleteScreenItems(scheduleViewScreen);
    }

    IrrigationSchedule schedule = schedules.getSchedule(scheduleIndex);
    int numberOfChannels = schedules.getNumberOfChannels();

    scheduleViewScreen->addItem(new MenuItem(strdup(("Start: " + schedule.getStartTimeString()).c_str())));
    scheduleViewScreen->addItem(new MenuItem(strdup(("Days: " + String(daysToRunValues[schedule.getDaysToRun()])).c_str())));
    scheduleViewScreen->addItem(new MenuItem(strdup(("Weight: " + String(schedule.getWeight()) + "%").c_str())));
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1) + ": " + String(schedule.getChannelDuration(i)) + " min";
        scheduleViewScreen->addItem(new MenuItem(strdup(itemName.c_str())));
    }
    scheduleViewScreen->addItem(new ItemCommandInt("Edit", scheduleIndex, commandScheduleEditCallback));
    scheduleViewScreen->addItem(new ItemBack("Back"));
    Serial.printf("Schedule view submenu generated for index %d\n", scheduleIndex);
}

void Menu::GenerateScheduleEditSubmenu(int scheduleIndex)
{
    Serial.printf("Generating schedule edit submenu for index %d\n", scheduleIndex);
    if (scheduleEditScreen == nullptr)
    {
        scheduleEditScreen = new MenuScreen(std::vector<MenuItem *>());
        scheduleEditScreen->setParent(mainScreen);
    }
    else
    {
        DeleteScreenItems(scheduleEditScreen);
    }

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
    scheduleEditScreen->addItem(new ItemWidget<int, int>(strdup("Start"), new WidgetRange<int>(schedule.getStartTimeHours(), 1, 0, 23, " %02d", 0, true), new WidgetRange<int>(schedule.getStartTimeMinutes(), 15, 0, 45, ":%02d", 0, true), nullptr));
    scheduleEditScreen->addItem(new ItemWidget<uint8_t>(strdup("Days"), new WidgetList<const char *>(daysToRunValues, (int)schedule.getDaysToRun(), " %s", 0, true, nullptr), nullptr));
    scheduleEditScreen->addItem(new ItemWidget<int>(strdup("Weight"), new WidgetRange<int>(schedule.getWeight(), 25, 50, 150, " %d%%", 1, false), nullptr));
    for (int i = 0; i < numberOfChannels; i++)
    {
        String itemName = "Ch" + String(i + 1);
        scheduleEditScreen->addItem(new ItemWidget<int>(strdup(itemName.c_str()), new WidgetRange<int>(schedule.getChannelDuration(i), 1, manualIrrigationDurationMin, manualIrrigationDurationMax, " %d min", 4, false), nullptr));
    }
    scheduleEditScreen->addItem(new ItemCommandInt("Save", scheduleIndex, commandScheduleSaveCallback));
    scheduleEditScreen->addItem(new ItemCommandInt("Delete", scheduleIndex, commandScheduleDeleteCallback));
    scheduleEditScreen->addItem(new ItemBack("Back"));
    Serial.println("Schedule edit submenu generated");
}

void Menu::DeleteScreenItems(MenuScreen *screen)
{
    Serial.println("Deleting screen items");
    if (screen != nullptr)
    {
        while (screen->size() > 0)
        {
            screen->removeLastItem();
        }
    }
    else
    {
        Serial.println("Screen is null, nothing to delete.");
    }
}
