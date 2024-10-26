#include "menu.h"
#include "statuses.h"

Menu *Menu::instance = nullptr;

Menu::Menu(int rotaryEncoderPin1, int rotaryEncoderPin2, int rotaryEncoderButton)
{
    instance = this;
    rotaryEncoder = new RotaryEncoder(rotaryEncoderPin1, rotaryEncoderPin2, rotaryEncoderButton);

    rotaryEncoder->setEncoderType(EncoderType::HAS_PULLUP);
    rotaryEncoder->setBoundaries(1, 10, true);
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
}

void Menu::buttonCallback(unsigned long duration)
{
    // This gets executed every time the pushbutton is pressed
    instance->rotaryEncoderButtonDuration = duration;
    if (duration > 1000)
    {
        MenuStatusL1 = 0;
        MenuStatusL2 = 0;
        MenuStatusL3 = 0;
        MenuPosition = 0;
        instance->rotaryEncoder->setBoundaries(1, 10, true);
        instance->rotaryEncoderPosition = 0;
        return;
    }

    if (MenuStatusL1 == 0)
    {
        MenuStatusL1 = instance->rotaryEncoderPosition;
    }
    else if (MenuStatusL2 == 0)
    {
        if (MenuStatusL1 == 1)
        {
            MenuStatusL2 = instance->rotaryEncoderPosition;
        }
        else if (MenuStatusL1 == 2)
        {
            MenuStatusL2 = instance->rotaryEncoderPosition;
        }
        else if (MenuStatusL1 == 3)
        {
            MenuStatusL2 = instance->rotaryEncoderPosition;
        }
    }
    else if (MenuStatusL3 == 0)
    {
        if (MenuStatusL1 == 1)
        {
            if (MenuStatusL2 == 1)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 2)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 3)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
        }
        else if (MenuStatusL1 == 2)
        {
            if (MenuStatusL2 == 1)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 2)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 3)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
        }
        else if (MenuStatusL1 == 3)
        {
            if (MenuStatusL2 == 1)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 2)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
            }
            else if (MenuStatusL2 == 3)
            {
                MenuStatusL3 = instance->rotaryEncoderPosition;
                instance->rotaryEncoder->setBoundaries(1, 30, false);
            }
        }
    }

    instance->rotaryEncoderPosition = 0;
}
