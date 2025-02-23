#include "actions.h"
#include "statuses.h"

Actions *Actions::instance = nullptr;

Actions::Actions(int rotaryEncoderPin1, int rotaryEncoderPin2, int rotaryEncoderButton)
{
    instance = this;
    rotaryEncoder = new RotaryEncoder(rotaryEncoderPin1, rotaryEncoderPin2, rotaryEncoderButton);

    rotaryEncoder->setEncoderType(EncoderType::HAS_PULLUP);
    rotaryEncoder->setBoundaries(1, 9, true);
    rotaryEncoder->onTurned(knobCallback);
    rotaryEncoder->onPressed(buttonCallback);
    rotaryEncoder->begin();
}

Actions::~Actions()
{
}

void Actions::knobCallback(long value)
{
    // This gets executed every time the knob is turned
    instance->rotaryEncoderPosition = value;
    MenuPosition = value;
    MenuStatusChanged = true;
}

void Actions::buttonCallback(unsigned long duration)
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

    instance->rotaryEncoder->setEncoderValue(0);
    instance->rotaryEncoderPosition = instance->rotaryEncoder->getEncoderValue();
}

void Actions::ResetMenu()
{
    MenuStatusL1 = 0;
    MenuStatusL2 = 0;
    MenuStatusL3 = 0;
    MenuPosition = 0;
    rotaryEncoder->setBoundaries(1, 9, true);
    rotaryEncoder->setEncoderValue(0);
    rotaryEncoderPosition = rotaryEncoder->getEncoderValue();
}
