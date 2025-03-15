#ifndef DISPLAY_H
#define DISPLAY_H

#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <vector>
#include "settings.h"

class Display
{
public:
    Display(const std::vector<int>& pins, int dimmPin);
    void DisplayDimm(int value);
    void DisplayText();
    void DisplayMessage(String message, bool first = true, bool last = false);
    void DisplayActivate(int timeout = DISPLAY_TIMEOUT_INTERVAL);
    LiquidCrystal_I2C GetLcd();
private:
    int numOfChannels;
    int *channelPins;
    int displayDimmPin;
    bool timeSynced = false;
    String displayLinesText[DISPLAY_LINES - 1] = {"", "", ""};
    int displayLinesPosition[DISPLAY_LINES - 1] = {0, 0, 0};
    long displayTimeout = 0;
    unsigned long displayLastUpdate = 0;
    unsigned long displayLastActivity = 0;
    bool clearingNeeded = false;

    void HandleTimeouts(int elapsed);
    void DisplayStatus(int animation);
    void DisplayBigNumber(int row, int column, int num, bool colon = false);
    void DisplayMenu(int dispRow, int l1, int l2, int l3);
    void DisplayMenu(int dispRow, String menuItems[], int menuItemsCount);
    void DisplayMenu1(int dispRow);
    void DisplayWifiStatus(int dispRow);
    String LongToString(long value, int digits);
};

#endif // DISPLAY_H