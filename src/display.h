#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <vector>

// Constants for display
const int displayColumns = 20;
const int displayLines = 4;
const unsigned long displayTimeoutInterval = 5000;
const unsigned long displayLastUpdateInterval = 300;

class Display
{
public:
    Display(const std::vector<int>& pins, int dimmPin);
    void DisplayBigNumber(int row, int column, int num, bool colon = false);
    void DisplayStatus(int animation);
    void HandleTimeouts(int elapsed);
    void DisplayDimm(int value);
    void DisplayText();
    void DisplayMessage(String message, int row, bool first = true, bool last = false);
private:
    int numOfChannels;
    int *channelPins;
    int displayDimmPin;
    bool timeSynced = false;
    String displayLinesText[displayLines - 1] = {"", "", ""};
    int displayLinesPosition[displayLines - 1] = {0};
    long displayTimeout = 0;
    unsigned long displayLastUpdate = 0;
    bool clearingNeeded = false;
    String LongToString(long value, int digits);
};

#endif // DISPLAY_H