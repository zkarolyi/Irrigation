#include "menu.h"
#include <cstddef>
#include <vector>
#include <string>

const int MenuDisplayMaxItems = 9;

// Submenus
const MenuItem submenu11[] = {
    {NULL, 0, 10, {}, NULL}, // Submenu 1.1.1
    {NULL, 0, 20, {}, NULL}, // Submenu 1.1.2
    {NULL, 0, 30, {}, NULL}, // Submenu 1.1.3
};

const MenuItem submenu12[] = {
    {NULL, 0, 0, {}, NULL}, // Submenu 1.2.1
    {NULL, 0, 0, {}, NULL}, // Submenu 1.2.2
    {NULL, 0, 0, {}, NULL}, // Submenu 1.2.3
};

const MenuItem submenu13[] = {
    {NULL, 0, 0, {}, NULL}, // Submenu 1.3.1
    {NULL, 0, 0, {}, NULL}, // Submenu 1.3.2
    {NULL, 0, 0, {}, NULL}, // Submenu 1.3.3
};

const MenuItem submenu1[] = {
    {submenu11, 3, 3, {}, NULL}, // Submenu 1.1
    {submenu12, 3, 3, {}, NULL}, // Submenu 1.2
    {submenu13, 3, 3, {}, NULL}, // Submenu 1.3
};

const MenuItem submenu2[] = {
    {NULL, 0, 0, {}, NULL}, // Submenu 2.1
};

// Main menu
const MenuItem mainMenu[] = {
    {submenu1, 2, 2, {"Wifi information", "Menu item 2", "Menu item 3"}, NULL}, // Menu 1
    {submenu2, 1, 1, {}, NULL}, // Menu 2
    {NULL, 0, 0, {}, NULL},     // Menu 3
};

MenuItem GetMenuItem(int L1, int L2, int L3)
{
    const MenuItem *currentMenu = mainMenu;
    if (L1 == 0)
    {
        return *currentMenu;
    }
    else
    {
        currentMenu = &currentMenu->subMenu[L1 - 1];
    }

    if (L2 == 0)
    {
        return *currentMenu;
    }
    else
    {
        currentMenu = &currentMenu->subMenu[L2 - 1];
    }

    if (L3 == 0)
    {
        return *currentMenu;
    }
    else
    {
        currentMenu = &currentMenu->subMenu[L3 - 1];
    }

    return *currentMenu;
}