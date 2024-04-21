void handle_OnRoot();
void handle_NotFound();
String SendHTML(float temperature, float humidity, float pressure, float altitude);
void GetFile(String fileName);
String GetHtml(bool ch1, bool ch2, bool ch3, bool ch4, bool ch5, bool ch6);
void InitFS();

uint8_t check[8] = {0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000};
uint8_t cross[8] = {0b00000, 0b11011, 0b01110, 0b00100, 0b01110, 0b11011, 0b00000};
uint8_t retarrow[8] = {0b00001, 0b00001, 0b00101, 0b01001, 0b11111, 0b01000, 0b00100};
String displayLinesText[] = {"", ""};
int displayLinesPosition[] = {0, 0};
int displayLastUpdate = 0;
int displayTimeout = -1;
int displayLastUpdateInterval = 400;