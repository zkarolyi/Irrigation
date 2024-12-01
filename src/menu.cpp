#include "menu.h"
#include "statuses.h"

Menu *Menu::instance = nullptr;

typedef struct MenuItem
{
    const struct MenuItem *subMenu; // Submenu pointer
    const int subMenuCount;         // submenu number
    const int boundaries;           // Menu boundaries
    void (*action)(void);           // Function pointer (optional)
} MenuItem;

// Submenus
const MenuItem submenu11[] = {
    {NULL, 0, 10, NULL}, // Submenu 1.1.1
    {NULL, 0, 20, NULL}, // Submenu 1.1.2
    {NULL, 0, 30, NULL}, // Submenu 1.1.3
};

const MenuItem submenu12[] = {
    {NULL, 0, 0, NULL}, // Submenu 1.2.1
    {NULL, 0, 0, NULL}, // Submenu 1.2.2
    {NULL, 0, 0, NULL}, // Submenu 1.2.3
};

const MenuItem submenu13[] = {
    {NULL, 0, 0, NULL}, // Submenu 1.3.1
    {NULL, 0, 0, NULL}, // Submenu 1.3.2
    {NULL, 0, 0, NULL}, // Submenu 1.3.3
};

const MenuItem submenu1[] = {
    {submenu11, 3, 3, NULL}, // Submenu 1.1
    {submenu12, 3, 3, NULL}, // Submenu 1.2
    {submenu13, 3, 3, NULL}, // Submenu 1.3
};

const MenuItem submenu2[] = {
    {NULL, 0, 0, NULL}, // Submenu 2.1
};

// Main menu
const MenuItem mainMenu[] = {
    {submenu1, 2, 2, NULL}, // Menu 1
    {submenu2, 1, 1, NULL}, // Menu 2
    {NULL, 0, 0, NULL},     // Menu 3
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

// typedef struct MenuItem {
//     const struct MenuItem* subMenu;
//     const int subMenuCount;
// } MenuItem;

// const MenuItem submenu1[] = {
//     { NULL, 0 },
//     { NULL, 0 },
// };

// const MenuItem submenu2[] = {
//     { NULL, 0 },
// };

// const MenuItem mainMenu[] = {
//     { submenu1, 2 },
//     { submenu2, 1 },
//     { NULL, 0 },
// };

Menu::Menu(int rotaryEncoderPin1, int rotaryEncoderPin2, int rotaryEncoderButton)
{
    instance = this;
    rotaryEncoder = new RotaryEncoder(rotaryEncoderPin1, rotaryEncoderPin2, rotaryEncoderButton);

    rotaryEncoder->setEncoderType(EncoderType::HAS_PULLUP);
    rotaryEncoder->setBoundaries(1, 9, true);
    rotaryEncoder->onTurned(knobCallback);
    rotaryEncoder->onPressed(buttonCallback);
    rotaryEncoder->begin();
}

Menu::~Menu()
{
}

void Menu::knobCallback(long value)
{
    // This gets executed every time the knob is turned
    instance->rotaryEncoderPosition = value;
    MenuPosition = value;
    MenuStatusChanged = true;
}

void Menu::buttonCallback(unsigned long duration)
{
    // This gets executed every time the pushbutton is pressed
    instance->rotaryEncoderButtonDuration = duration;
    MenuStatusChanged = true;
    if (duration > 1000)
    {
        instance->ResetMenu();
        return;
    }

    // Aktuális szintű menü feldolgozása
    if (MenuStatusL1 == 0)
    {
        MenuStatusL1 = instance->rotaryEncoderPosition;
    }
    else if (MenuStatusL2 == 0)
    {
        MenuStatusL2 = instance->rotaryEncoderPosition;
    }
    else if (MenuStatusL2 == 0)
    {
        MenuStatusL3 = instance->rotaryEncoderPosition;
    }

    MenuItem currentMenuItem = GetMenuItem(MenuStatusL1, MenuStatusL2, MenuStatusL3);
    if (currentMenuItem.boundaries != 0)
    {
        instance->rotaryEncoder->setBoundaries(1, currentMenuItem.boundaries, false);
    }
    if (currentMenuItem.action != NULL)
    {
        currentMenuItem.action();
    }

    // if (MenuStatusL1 == 0)
    // {
    //     MenuStatusL1 = instance->rotaryEncoderPosition;
    // }
    // else if (MenuStatusL2 == 0)
    // {
    //     if (MenuStatusL1 == 1)
    //     {
    //         MenuStatusL2 = instance->rotaryEncoderPosition;
    //     }
    //     else if (MenuStatusL1 == 2)
    //     {
    //         MenuStatusL2 = instance->rotaryEncoderPosition;
    //     }
    //     else if (MenuStatusL1 == 3)
    //     {
    //         MenuStatusL2 = instance->rotaryEncoderPosition;
    //     }
    // }
    // else if (MenuStatusL3 == 0)
    // {
    //     if (MenuStatusL1 == 1)
    //     {
    //         if (MenuStatusL2 == 1)
    //         {
    //             instance->ResetMenu();
    //         }
    //         else if (MenuStatusL2 == 2)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //         else if (MenuStatusL2 == 3)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //     }
    //     else if (MenuStatusL1 == 2)
    //     {
    //         if (MenuStatusL2 == 1)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //         else if (MenuStatusL2 == 2)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //         else if (MenuStatusL2 == 3)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //     }
    //     else if (MenuStatusL1 == 3)
    //     {
    //         if (MenuStatusL2 == 1)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //         else if (MenuStatusL2 == 2)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //         }
    //         else if (MenuStatusL2 == 3)
    //         {
    //             MenuStatusL3 = instance->rotaryEncoderPosition;
    //             instance->rotaryEncoder->setBoundaries(1, 30, false);
    //         }
    //     }
    // }

    instance->rotaryEncoder->setEncoderValue(0);
    instance->rotaryEncoderPosition = instance->rotaryEncoder->getEncoderValue();
}

void Menu::ResetMenu()
{
    MenuStatusL1 = 0;
    MenuStatusL2 = 0;
    MenuStatusL3 = 0;
    MenuPosition = 0;
    rotaryEncoder->setBoundaries(1, 9, true);
    rotaryEncoder->setEncoderValue(0);
    rotaryEncoderPosition = rotaryEncoder->getEncoderValue();
}
