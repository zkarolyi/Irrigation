void handle_OnRoot();
void handle_OnToggleSwitch();
void handle_NotFound();
String SendHTML(float temperature, float humidity, float pressure, float altitude);
void GetFile(String fileName);
String GetHtml(bool ch[]);
void InitFS();

const char *HOSTNAME = "IrrigationController";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

std::vector<int> relayPins = {12, 14, 27, 26, 25, 23, 32, 13};
bool irrigationScheduleEnabled = true;
int irrigationCheckInterval = 10000;
int irrigationLastCheck = 0;

int displayNetworkActivity = 0;
int displayOutChange = 0;

const char *schedulesFile = "/schedules.json";

int rotaryEncoderPin1 = 34;
int rotaryEncoderPin2 = 35;
int rotaryEncoderButton = 15;
long rotaryEncoderPosition = 0;
unsigned long rotaryEncoderButtonDuration = 0;

String LongToString(long number, int digits)
{
    char buffer[digits + 1];
    sprintf(buffer, "%0*d", digits, number);
    return String(buffer);
}