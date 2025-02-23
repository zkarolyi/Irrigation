#ifndef ACTIONS_H
#define ACTIONS_H

#include <ESP32RotaryEncoder.h>
#include "menu.h"

class Actions
{
public:
    Actions(int rotaryEncoderPin1,int rotaryEncoderPin2,int rotaryEncoderButton);
    ~Actions();
    void ResetMenu();
    static Actions* getInstance() { return instance; } 
private:
    static void knobCallback(long value);           // Static callback for knob rotation
    static void buttonCallback(unsigned long duration); // Static callback for button press

    static Actions *instance;

    RotaryEncoder *rotaryEncoder;
    long rotaryEncoderPosition;
    unsigned long rotaryEncoderButtonDuration;
};

#endif