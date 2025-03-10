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

Menu::Menu()
    : lcdAdapter(new LiquidCrystal_I2C(DISPLAY_ADDRESS, DISPLAY_COLUMNS, DISPLAY_LINES)),
      renderer(CharacterDisplayRenderer(&lcdAdapter, DISPLAY_COLUMNS, DISPLAY_LINES, 0x7E, 0x7F, NULL, NULL)),
      menu(LcdMenu(renderer)),
      encoder(ROTARY_ENCODER_PIN1, ROTARY_ENCODER_PIN2, ROTARY_ENCODER_BUTTON),
      rotaryInput(&menu, &encoder)
{
    Serial.println("Menu initialized");
}

void Menu::GenerateIrrigationSubmenu(int numberOfSchedules)
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
    // delete[] items;
}

// // Function to dynamically create a menu based on a parameter string list
// MenuScreen *createMenu(const std::vector<std::string> &itemNames)
// {
//     // Create a vector to hold the MenuItem pointers
//     std::vector<MenuItem *> items;

//     // Iterate over the item names and create MenuItem objects
//     for (const auto &name : itemNames)
//     {
//         items.push_back(new MenuItem(name.c_str()));
//     }

//     // Add a nullptr to the end of the items array
//     items.push_back(nullptr);

//     // Create a MenuScreen with the items array
//     MenuScreen *screen = new MenuScreen(items.data());

//     return screen;
// }

// // Example usage
// https://github.com/forntoh/LcdMenu/issues/291
//     // Example item names
//     std::vector<std::string> itemNames = {"Test1", "Test2"};
//     // Create the menu screen
//     MenuScreen* screen = createMenu(itemNames);
//     // Add the screen to the menu
//     menu.addScreen(screen);
