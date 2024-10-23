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
const int displayDimmPin = 5;

class Display
{
public:
    Display(const std::vector<int>& pins);
    void DisplayBigNumber(int row, int column, int num, bool colon = false);
    void DisplayStatus(int animation);
    void HandleTimeouts(int elapsed);
    void DisplayText();
    void DisplayMessage(String message, int row, bool first = true, bool last = false);
private:
    int numOfChannels;
    int *channelPins;
    // bool irrigationEnabled = true;
    // int displayNetworkActivity = 0;
    // int displayOutChange = 0;
    String displayLinesText[displayLines - 1] = {"", "", ""};
    int displayLinesPosition[displayLines - 1] = {0};
    unsigned long displayTimeout = -1;
    unsigned long displayLastUpdate = 0;
};

#endif // DISPLAY_H