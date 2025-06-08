#ifndef PARAMETERS_H
#define PARAMETERS_H

const int ROTARY_ENCODER_PIN1 = 34;
const int ROTARY_ENCODER_PIN2 = 35;
const int ROTARY_ENCODER_BUTTON = 15;

// RTC DS3231 I2C address
const int RTC_ADDRESS = 0x68;
const int RTC_EEPROM_ADDRESS = 0x57;

const int DISPLAY_ADDRESS = 0x27;
const int DISPLAY_COLUMNS = 20;
const int DISPLAY_LINES = 4;
const unsigned long DISPLAY_TIMEOUT_INTERVAL = 5000; // 5 seconds
const unsigned long DISPLAY_LAST_UPDATE_INTERVAL = 300; // 300 milliseconds

const unsigned long TIMER_TIMEOUT_INTERVAL = 1000 * 60 * 60; // 1 hour

const int RELAY_PIN_1 = 13;
const int RELAY_PIN_2 = 32;
const int RELAY_PIN_3 = 23;
const int RELAY_PIN_4 = 25;
const int RELAY_PIN_5 = 26;
const int RELAY_PIN_6 = 27;
const int RELAY_PIN_7 = 14;
const int RELAY_PIN_8 = 12;
// std::vector<int> relayPins = {12, 14, 27, 26, 25, 23, 32, 13};

#ifdef DOUBLE_PRESS_THRESHOLD
#undef DOUBLE_PRESS_THRESHOLD
#endif
#define DOUBLE_PRESS_THRESHOLD 500

#endif // PARAMETERS_H
