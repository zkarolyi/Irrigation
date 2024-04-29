void handle_OnRoot();
void handle_NotFound();
String SendHTML(float temperature, float humidity, float pressure, float altitude);
void GetFile(String fileName);
String GetHtml(bool ch1, bool ch2, bool ch3, bool ch4, bool ch5, bool ch6);
void InitFS();

// uint8_t check[8] = {0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000};
// uint8_t cross[8] = {0b00000, 0b11011, 0b01110, 0b00100, 0b01110, 0b11011, 0b00000};
// uint8_t retarrow[8] = {0b00001, 0b00001, 0b00101, 0b01001, 0b11111, 0b01000, 0b00100};
// the 8 arrays that form each segment of the custom numbers
byte LT[8] PROGMEM = {B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111};
byte UB[8] PROGMEM = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
byte RT[8] PROGMEM = {B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111};
byte LL[8] PROGMEM = {B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111};
byte LB[8] PROGMEM = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
byte LR[8] PROGMEM = {B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100};
byte MB[8] PROGMEM = {B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111};

byte bigNumbers[10][6] PROGMEM = {
    {0, 1, 2, 3, 4, 5},       // 0
    {1, 2, 254, 254, 255, 254}, // 1
    {6, 6, 2, 255, 4, 4},       // 2
    {6, 6, 2, 4, 4, 5},       // 3
    {3, 4, 2, 254, 254, 5},   // 4
    {255, 6, 6, 4, 4, 5},       // 5
    {0, 6, 6, 3, 4, 5},       // 6
    {1, 1, 2, 254, 0, 254},   // 7
    {0, 6, 2, 3, 4, 5},       // 8
    {0, 6, 2, 4, 4, 5}    // 9
};

const int displayLines = 2;
int displayColumns = 16;
String displayLinesText[displayLines] = {};
int displayLinesPosition[displayLines] = {};
int displayLastUpdate = 0;
int displayTimeout = -1;
int displayLastUpdateInterval = 300;
int displayTimeoutInterval = 5000;

const char *HOSTNAME = "IrrigationController";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

int relayPins[] = {12, 14, 27, 26, 25, 33, 32, 13};