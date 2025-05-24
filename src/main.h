void handle_OnRoot();
void handle_OnToggleSwitch();
void handle_NotFound();
void InitFS();

const char *HOSTNAME = "Irrigation";

const char *ntpServer = "pool.ntp.org";
// const long gmtOffset_sec = 3600;
const char* tz = "CET-1CEST,M3.5.0/2,M10.5.0/3";
// const int DST_START_MONTH = 3;    // March
// const int DST_START_WEEKDAY = 0;  // Sunday (0=Sunday)
// const int DST_START_WEEK = -1;    // Last week of the month
// const int DST_START_HOUR = 1;     // 01:00 UTC
// const int DST_END_MONTH = 10;     // October
// const int DST_END_WEEKDAY = 0;    // Sunday (0=Sunday)
// const int DST_END_WEEK = -1;      // Last week of the month
// const int DST_END_HOUR = 1;       // 01:00 UTC

bool irrigationScheduleEnabled = true;
int irrigationCheckInterval = 10000;
int irrigationLastCheck = 0;

int displayDimmPin = 5;
int displayNetworkActivity = 0;
int displayOutChange = 0;

int timerTimeout = 0;

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