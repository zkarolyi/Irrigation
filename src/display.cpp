#include "display.h"
#include "settings.h"
#include "globals.h"

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
LiquidCrystal_I2C lcd(DISPLAY_ADDRESS, DISPLAY_COLUMNS, DISPLAY_LINES);

// Constructor
Display::Display(const std::vector<int> &pins, int dimmPin)
{
    displayDimmPin = dimmPin;

    lcd.init();
    lcd.clear();
    lcd.backlight();
    pinMode(displayDimmPin, OUTPUT);
    analogWrite(displayDimmPin, 255);

    delay(1000);

    this->SetCustomChars();

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

// LiquidCrystal_I2C& Display::GetLcd()
// {
//     return lcd;
// }

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

void Display::DisplayWifiStatus(int dispRow)
{
    if (dispRow == 1)
    {
        lcd.setCursor(0, 0);
        lcd.print(wifiSsid);
        lcd.setCursor(0, 1);
        lcd.print("IP ");
        lcd.print(wifiIpAddress);
        lcd.setCursor(0, 2);
        lcd.print(wifiMacAddress);
    }
    else if (dispRow == 2)
    {
        lcd.setCursor(0, 0);
        lcd.print("DNS ");
        lcd.print(wifiDnsIp);
        lcd.setCursor(0, 1);
        lcd.print("GW ");
        lcd.print(wifiGatewayIp);
        lcd.setCursor(0, 2);
        lcd.print(wifiHostname);
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

    lcd.setCursor(0, DISPLAY_LINES - 1);
    for (int i = 0; i < numOfChannels; i++)
    {
        lcd.write((digitalRead(channelPins[i]) == HIGH ? '_' : animationChar)); // Update with actual pin logic
    }

    if (irrigationManualEnd > millis())
    {
        lcd.print(" - ");
        int left = static_cast<int>((irrigationManualEnd - millis()) / 1000);
        char timeLeft[6];
        snprintf(timeLeft, sizeof(timeLeft), "%02d:%02d", left / 60, left % 60);
        lcd.print(timeLeft);
    }

    // Network activity status
    lcd.setCursor(DISPLAY_COLUMNS - 1, 0);
    lcd.write(displayNetworkActivity > 0 ? B11001110 : ' ');

    // Schedule status
    lcd.setCursor(DISPLAY_COLUMNS - 1, 1);
    lcd.write(irrigationScheduleEnabled ? B11001001 : 'M');

    // Output change status
    lcd.setCursor(DISPLAY_COLUMNS - 1, 2);
    lcd.write(displayOutChange > 0 ? 'o' : ' ');

    // Rain sensor status
    lcd.setCursor(DISPLAY_COLUMNS - 1, 3);
    lcd.write(timeSynced ? 'Y' : B11011110);
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

void Display::DisplayDimm(int value)
{
    analogWrite(displayDimmPin, value);
}

void Display::DisplayActivate(int timeout)
{
    displayTimeout = timeout;
    displayLastActivity = millis();
    displayLastScroll = millis();
}

void Display::SetCustomChars()
{
    lcd.createChar(0, LT);
    lcd.createChar(1, UB);
    lcd.createChar(2, RT);
    lcd.createChar(3, LL);
    lcd.createChar(4, LB);
    lcd.createChar(5, LR);
    lcd.createChar(6, MB);
    lcd.createChar(7, BS);
}

void Display::DisplayText()
{
    if (millis() - displayLastUpdate < DISPLAY_LAST_UPDATE_INTERVAL)
    {
        return;
    }

    if (millis() - displayLastScroll > 10000)
    {
        for (int i = 0; i < DISPLAY_LINES - 2; i++)
        {
            displayLinesText[i] = displayLinesText[i + 1];
            displayLinesPosition[i] = displayLinesPosition[i + 1];
        }
        displayLinesText[DISPLAY_LINES - 2] = "";
        displayLastScroll = millis();
    }
    unsigned long elapsed = (millis() - displayLastActivity) / 1000; // Convert to seconds
    DisplayDimm(elapsed < 60 ? 250 - elapsed * 4 : 10);

    displayLastUpdate = millis();
    HandleTimeouts(DISPLAY_LAST_UPDATE_INTERVAL);

    DateTime now;
    if (rtc.begin() && !rtc.lostPower())
    {
        now = rtc.now();
        timeSynced = now.isValid();
    }
    else
    {
        timeSynced = false;
    }

    displayTimeout -= DISPLAY_LAST_UPDATE_INTERVAL;

    if (displayTimeout > 0)
    {
        clearingNeeded = true;
        for (int i = 0; i < DISPLAY_LINES - 1; i++)
        {
            lcd.setCursor(0, i);
            if (displayLinesText[i].length() <= DISPLAY_COLUMNS - 1)
            {
                lcd.print(displayLinesText[i]);
                for (int j = displayLinesText[i].length(); j < DISPLAY_COLUMNS - 1; j++)
                {
                    lcd.print(" ");
                }
            }
            else
            {
                if (displayLinesPosition[i] + DISPLAY_COLUMNS - 1 > displayLinesText[i].length())
                {
                    lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesText[i].length()));
                    lcd.print(displayLinesText[i].substring(0, DISPLAY_COLUMNS - 1 - (displayLinesText[i].length() - displayLinesPosition[i])));
                }
                else
                {
                    lcd.print(displayLinesText[i].substring(displayLinesPosition[i], displayLinesPosition[i] + DISPLAY_COLUMNS - 1));
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
        displayTimeout = 0;
        if (clearingNeeded)
        {
            clearingNeeded = false;
            lcd.clear();
        }
        if (timeSynced)
        {
            DisplayBigNumber(1, 2, now.hour() * 100 + now.minute(), now.second() % 2 == 0);
        }
    }
    DisplayStatus(timeSynced ? now.second() : 0);
}

void Display::DisplayMessage(String message, bool first, bool last)
{
    displayTimeout = DISPLAY_TIMEOUT_INTERVAL;
    displayLastActivity = millis();
    Serial.println(message);

    int row = DISPLAY_LINES - 2;
    if (first)
    {
        for (int i = 0; i < row; i++)
        {
            displayLinesText[i] = displayLinesText[i + 1];
            displayLinesPosition[i] = displayLinesPosition[i + 1];
        }
        displayLinesText[row] = "";
    }

    displayLinesText[row] += message;

    if (last)
    {
        if (displayLinesText[row].length() > DISPLAY_COLUMNS - 1)
        {
            displayLinesText[row] += "   ";
        }
        while (displayLinesText[row].length() < DISPLAY_COLUMNS - 1)
        {
            displayLinesText[row] += " ";
        }
    }
    displayLinesPosition[row] = 0;
}

String Display::LongToString(long value, int digits)
{
    String strValue = String(value);
    int padLength = digits - strValue.length();
    while (strValue.length() < digits)
    {
        strValue = " " + strValue;
    }
    return strValue;
}