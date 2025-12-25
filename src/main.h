void handle_OnRoot();
void handle_OnToggleSwitch();
void handle_OnSetScheduled();
void handle_NotFound();
void InitFS();

const char *HOSTNAME = "Irrigation";

const char *ntpServer = "pool.ntp.org";
const char *tz = "CET-1CEST,M3.5.0/2,M10.5.0/3";

struct MqttConfig {
  const char *broker;
  int port;
  const char *username;
  const char *password;
  const char *topic;
  const char *clientId;
  bool isValid() const {
    return broker != nullptr && port > 0 && username != nullptr && password != nullptr && topic != nullptr && clientId != nullptr;
  }
  void free() {
    ::free((void*)broker);
    ::free((void*)username);
    ::free((void*)password);
    broker = nullptr;
    username = nullptr;
    password = nullptr;
  }
};
extern MqttConfig mqttConfig;
void sendMQTTMessage(String path, String payload);
void mqttMessageHandler(char *topic, byte *payload, unsigned int length);
void startChannel(int channel, int duration);
void stopChannel(int channel);
void toggleChannel(int channel, int duration);
boolean readMqttCredentials();

bool irrigationScheduleEnabled = true;
int irrigationCheckInterval = 10000;
int irrigationLastCheck = 0;
int irrigationManualEnd = 0;

int displayDimmPin = 5;
int displayNetworkActivity = 0;
int displayOutChange = 0;

unsigned long TimerLastRun = 0;

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
