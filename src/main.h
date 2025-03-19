void handle_OnRoot();
void handle_OnToggleSwitch();
void handle_NotFound();
void InitFS();

const char *HOSTNAME = "Irrigation";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0; //3600;

bool irrigationScheduleEnabled = true;
int irrigationCheckInterval = 10000;
int irrigationLastCheck = 0;

int displayDimmPin = 5;
int displayNetworkActivity = 0;
int displayOutChange = 0;

String wifiIpAddress;
String wifiDnsIp;
String wifiGatewayIp;
String wifiHostname;
String wifiMacAddress;
String wifiSsid;

void InitializeWebServer();
void handle_OnGetSchedule();
void handle_OnSetSchedule();
void handle_OnSetDimming();
void handle_onScheduleList();
void handle_OnGetSettings();

const char *schedulesFile = "/schedules.json";

String LongToString(long number, int digits)
{
    char buffer[digits + 1];
    sprintf(buffer, "%0*d", digits, number);
    return String(buffer);
}