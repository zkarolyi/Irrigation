#include <vector>
#include <string>
#include "menu.h"
#include "parameters.h"
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <LcdMenu.h>
#include <SimpleRotary.h>
#include <ItemList.h>
#include <ItemWidget.h>
#include <ItemSubMenu.h>
#include <MenuItem.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
#include <input/SimpleRotaryAdapter.h>
// #include <input/KeyboardAdapter.h>
#include <widget/WidgetList.h>
#include <widget/WidgetRange.h>

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

// MENU_SCREEN(schedulesScreen, schedulesItems,
//             ITEM_BASIC("Backlight"),
//             ITEM_BASIC("Contrast"));
MenuScreen *schedulesScreen;

MENU_SCREEN(settingsScreen, settingsItems,
            ITEM_BASIC("Backlight"));

void toggleCallback(bool isOn, int index)
{
    Serial.printf("Channel %d %s\n", index, isOn ? "started" : "stopped");
}

void commandCallback()
{
    Serial.println("Callback");
}

Menu::Menu()
    : lcdAdapter(new LiquidCrystal_I2C(DISPLAY_ADDRESS, DISPLAY_COLUMNS, DISPLAY_LINES)),
      renderer(CharacterDisplayRenderer(&lcdAdapter, DISPLAY_COLUMNS, DISPLAY_LINES,0x7E,0x7F,NULL,NULL)),
      menu(LcdMenu(renderer)),
      encoder(ROTARY_ENCODER_PIN1, ROTARY_ENCODER_PIN2, ROTARY_ENCODER_BUTTON),
      rotaryInput(&menu, &encoder)
{
    Serial.println("Menu initialized");
}

void Menu::GenerateIrrigationSubmenu(int numberOfSchedules)
{
    // Allocate memory dynamically for the items array
    MenuItem **items = new MenuItem *[numberOfSchedules + 1];
    for (int i = 0; i < numberOfSchedules; i++)
    {
        String itemName = "Schedule " + String(i + 1);
        items[i] = new MenuItem(strdup(itemName.c_str()));
    }

    items[numberOfSchedules] = nullptr;

    // Delete the old schedulesScreen if it exists to avoid memory leaks
    // if (schedulesScreen != nullptr)
    // {
    //     delete schedulesScreen;
    // }

    schedulesScreen = new MenuScreen(items);

    // Free the dynamically allocated memory for items array
    //delete[] items;
}

// Function to dynamically create a menu based on a parameter string list
MenuScreen *createMenu(const std::vector<std::string> &itemNames)
{
    // Create a vector to hold the MenuItem pointers
    std::vector<MenuItem *> items;

    // Iterate over the item names and create MenuItem objects
    for (const auto &name : itemNames)
    {
        items.push_back(new MenuItem(name.c_str()));
    }

    // Add a nullptr to the end of the items array
    items.push_back(nullptr);

    // Create a MenuScreen with the items array
    MenuScreen *screen = new MenuScreen(items.data());

    return screen;
}

// // Example usage
// https://github.com/forntoh/LcdMenu/issues/291
//     // Example item names
//     std::vector<std::string> itemNames = {"Test1", "Test2"};
//     // Create the menu screen
//     MenuScreen* screen = createMenu(itemNames);
//     // Add the screen to the menu
//     menu.addScreen(screen);
