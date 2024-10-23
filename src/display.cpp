#include "display.h"
#include "statuses.h"

byte LT[8] PROGMEM = {B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111};
byte UB[8] PROGMEM = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
byte RT[8] PROGMEM = {B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111};
byte LL[8] PROGMEM = {B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111};
byte LB[8] PROGMEM = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
byte LR[8] PROGMEM = {B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100};
byte MB[8] PROGMEM = {B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111};

byte BS[8] PROGMEM = {B00000, B10000, B01000, B00100, B00010, B00001, B00000, B00000};

byte bigNumbers[10][6] PROGMEM = {
    {0, 1, 2, 3, 4, 5},         // 0
    {1, 2, 254, 254, 255, 254}, // 1
    {6, 6, 2, 255, 4, 4},       // 2
    {6, 6, 2, 4, 4, 5},         // 3
    {3, 4, 2, 254, 254, 5},     // 4
    {255, 6, 6, 4, 4, 5},       // 5
    {0, 6, 6, 3, 4, 5},         // 6
    {1, 1, 2, 254, 0, 254},     // 7
    {0, 6, 2, 3, 4, 5},         // 8
    {0, 6, 2, 4, 4, 5}          // 9
};


// LCD instance
LiquidCrystal_I2C lcd(0x27, displayColumns, displayLines);

// Constructor
Display::Display(const std::vector<int>& pins)
{
    lcd.init();
    lcd.clear();
    lcd.backlight();
    pinMode(displayDimmPin, OUTPUT);
    analogWrite(displayDimmPin, 255);

    lcd.createChar(0, LT);
    lcd.createChar(1, UB);
    lcd.createChar(2, RT);
    lcd.createChar(3, LL);
    lcd.createChar(4, LB);
    lcd.createChar(5, LR);
    lcd.createChar(6, MB);
    // lcd.createChar(0, check);
    // lcd.createChar(1, cross);
    // lcd.createChar(2, retarrow);
    lcd.createChar(7, BS);

    numOfChannels = pins.size();
    channelPins = new int[numOfChannels];

    int i = 0;
    for (int pin : pins)
    {
        if (i < numOfChannels)
        {
            channelPins[i++] = pin;
        }
    }
}

void Display::DisplayBigNumber(int row, int column, int num, bool colon)
{
    for (int i = 3; i >= 0; i--)
    {
        int digit = static_cast<int>(num / pow(10, i)) % 10;

        lcd.setCursor(column, row);
        lcd.write(bigNumbers[digit][0]);
        lcd.write(bigNumbers[digit][1]);
        lcd.write(bigNumbers[digit][2]);
        if (i == 2 && colon)
        {
            lcd.write(B10100101); // colon symbol
        }
        else
        {
            lcd.write(254); // blank space
        }

        lcd.setCursor(column, row + 1);
        lcd.write(bigNumbers[digit][3]);
        lcd.write(bigNumbers[digit][4]);
        lcd.write(bigNumbers[digit][5]);
        if (i == 2 && colon)
        {
            lcd.write(B10100101); // colon symbol
        }
        else
        {
            lcd.write(254); // blank space
        }

        column += 4; // Adjust for next digit placement
    }
}

void Display::DisplayStatus(int animation)
{
    char animationChar;
    switch (animation % 4)
    {
    case 0:
        animationChar = '|';
        break;
    case 1:
        animationChar = '/';
        break;
    case 2:
        animationChar = '-';
        break;
    case 3:
        animationChar = B00000111;
        break;
    }

    lcd.setCursor(0, displayLines - 1);
    for (int i = 0; i < numOfChannels; i++)
    {
        lcd.write((digitalRead(channelPins[i]) == HIGH ? '_' : animationChar)); // Update with actual pin logic
    }

    // Network activity status
    lcd.setCursor(displayColumns - 1, 0);
    lcd.write(displayNetworkActivity > 0 ? B11001110 : ' ');

    // Schedule status
    lcd.setCursor(displayColumns - 1, 1);
    lcd.write(irrigationScheduleEnabled ? B11001001 : 'M');

    // Output change status
    lcd.setCursor(displayColumns - 1, 2);
    lcd.write(displayOutChange > 0 ? 'o' : ' ');

    // Rain sensor status
    lcd.setCursor(displayColumns - 1, 3);
    lcd.write(B11011110);
}

void Display::HandleTimeouts(int elapsed)
{
    displayNetworkActivity -= elapsed;
    if (displayNetworkActivity < 0)
        displayNetworkActivity = 0;

    displayOutChange -= elapsed;
    if (displayOutChange < 0)
        displayOutChange = 0;
}

void Display::DisplayText()
{
    if (millis() - displayLastUpdate < displayLastUpdateInterval)
    {
        return;
    }
    displayLastUpdate = millis();
    HandleTimeouts(displayLastUpdateInterval);

    struct tm timeinfo;

    if (displayTimeout > 0)
    {
        displayTimeout -= displayLastUpdateInterval;
        if (displayTimeout > 0)
        {
            for (int i = 0; i < displayLines - 1; i++)
            {
                lcd.setCursor(0, i);
                if (displayLinesText[i].length() <= displayColumns - 1)
                {
                    lcd.print(displayLinesText[i]);
                    for (int j = displayLinesText[i].length(); j < displayColumns - 1; j++)
                    {
                        lcd.print(" ");
                    }
                }
                else
                {
                    if (displayLinesPosition[i] + displayColumns - 1 > displayLinesText[i].length())
                    {
                        lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesText[i].length()));
                        lcd.print(displayLinesText[i].substring(0, displayColumns - 1 - (displayLinesText[i].length() - displayLinesPosition[i])));
                    }
                    else
                    {
                        lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesPosition[i] + displayColumns - 1));
                    }
                    displayLinesPosition[i]++;
                    if (displayLinesPosition[i] >= displayLinesText[i].length())
                    {
                        displayLinesPosition[i] = 0;
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < displayLines - 1; i++)
            {
                displayLinesText[i] = "";
            }
            lcd.clear();
        }
    }
    else
    {
        if (!getLocalTime(&timeinfo))
        {
            displayLinesText[0] = "--/--/--";
        }
        DisplayBigNumber(1, 2, timeinfo.tm_hour * 100 + timeinfo.tm_min, timeinfo.tm_sec % 2 == 0);
    }
    DisplayStatus(timeinfo.tm_sec);
}

void Display::DisplayMessage(String message, int row, bool first, bool last)
{
    displayTimeout = displayTimeoutInterval;
    Serial.println(message);

    if (first)
    {
        displayLinesText[row] = "";
    }
    displayLinesText[row] += message;

    if (last)
    {
        if (displayLinesText[row].length() > displayColumns - 1)
        {
            displayLinesText[row] += "   ";
        }
        while (displayLinesText[row].length() < displayColumns - 1)
        {
            displayLinesText[row] += " ";
        }
    }
    displayLinesPosition[row] = 0;
    DisplayText();
}
