#include <ESP32RotaryEncoder.h>
#ifndef MENU_H
#define MENU_H

class Menu
{
public:
    Menu(int rotaryEncoderPin1,int rotaryEncoderPin2,int rotaryEncoderButton);
    ~Menu();

private:
    static void knobCallback(long value);           // Static callback for knob rotation
    static void buttonCallback(unsigned long duration); // Static callback for button press

    static Menu *instance;

    RotaryEncoder *rotaryEncoder;
    long rotaryEncoderPosition;
    unsigned long rotaryEncoderButtonDuration;

    void ResetMenu();
};

#endif