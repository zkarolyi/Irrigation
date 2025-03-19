#ifndef PARAMETERS_H
#define PARAMETERS_H

const int ROTARY_ENCODER_PIN1 = 34;
const int ROTARY_ENCODER_PIN2 = 35;
const int ROTARY_ENCODER_BUTTON = 15;

const int DISPLAY_ADDRESS = 0x27;
const int DISPLAY_COLUMNS = 20;
const int DISPLAY_LINES = 4;
const unsigned long DISPLAY_TIMEOUT_INTERVAL = 5000;
const unsigned long DISPLAY_LAST_UPDATE_INTERVAL = 300;

const int RELAY_PIN_1 = 12;
const int RELAY_PIN_2 = 14;
const int RELAY_PIN_3 = 27;
const int RELAY_PIN_4 = 26;
const int RELAY_PIN_5 = 25;
const int RELAY_PIN_6 = 23;
const int RELAY_PIN_7 = 32;
const int RELAY_PIN_8 = 13;
// std::vector<int> relayPins = {12, 14, 27, 26, 25, 23, 32, 13};

#ifdef DOUBLE_PRESS_THRESHOLD
#undef DOUBLE_PRESS_THRESHOLD
#endif
#define DOUBLE_PRESS_THRESHOLD 500

#endif // PARAMETERS_H
