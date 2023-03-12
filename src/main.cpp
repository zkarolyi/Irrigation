#include <WiFi.h>
#include <WebServer.h>
#include <main.h>
#include "SPIFFS.h"

using namespace std;

float temperature, humidity, pressure, altitude;
bool ch1, ch2, ch3, ch4, ch5, ch6;

/*Put your SSID & Password*/
const char *ssid = "Csepp2";       // Enter SSID here
const char *password = "Karolyi1"; // Enter Password here

vector<String> v;

WebServer server(80);

void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println("Connecting to ");
  Serial.println(ssid);

  // connect to your local wi-fi network
  WiFi.begin(ssid, password);

  // check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  InitFS();
  GetFile("/Index.html");

  server.on("/", handle_OnRoot);
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  // int hallValue = hallRead();
  // Serial.print("Hall: ");
  // Serial.println(hallValue);
  // delay(100);

  server.handleClient();
}

void handle_OnRoot()
{
  Serial.println("Starting handle request");
  ch1 = false;
  ch2 = false;
  ch3 = true;
  ch4 = false;
  ch5 = false;
  ch6 = false;
  String body = GetHtml(ch1, ch2, ch3, ch4, ch5, ch6);
  server.send(200, "text/html", body);
  Serial.println("Finished");
}

// void handle_OnRoot()
// {
//   Serial.println("Starting handle request");
//   temperature = 100;
//   humidity = 110;
//   pressure = 120;
//   altitude = 130;
//   server.send(200, "text/html", SendHTML(temperature, humidity, pressure, altitude));
//   Serial.print(temperature);
//   Serial.print(humidity);
//   Serial.print(pressure);
//   Serial.println(altitude);
// }

void handle_NotFound()
{
  server.send(404, "text/plain", "Page not found");
}

String GetHtml(bool ch1, bool ch2, bool ch3, bool ch4, bool ch5, bool ch6)
{
  String ptr;
  for (String s : v)
  {
    ptr += s;
  }

  ptr.replace("Ch1", ch1 ? "X" : "_");
  ptr.replace("Ch2", ch2 ? "X" : "_");
  ptr.replace("Ch3", ch3 ? "X" : "_");
  ptr.replace("Ch4", ch4 ? "X" : "_");
  ptr.replace("Ch5", ch5 ? "X" : "_");
  ptr.replace("Ch6", ch6 ? "X" : "_");

  return ptr;
}

String SendHTML(float temperature, float humidity, float pressure, float altitude)
{
  String ptr;
  for (String s : v)
  {
    ptr += s;
  }

  return ptr;

  // String ptr = "<!DOCTYPE html> <html>\n";
  // ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  // ptr += "<title>ESP32 Weather Station</title>\n";
  // ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  // ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  // ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  // ptr += "</style>\n";
  // ptr += "</head>\n";
  // ptr += "<body>\n";
  // ptr += "<div id=\"webpage\">\n";
  // ptr += "<h1>ESP32 Weather Station</h1>\n";
  // ptr += "<p>Temperature: ";
  // ptr += temperature;
  // ptr += "&deg;C</p>";
  // ptr += "<p>Humidity: ";
  // ptr += humidity;
  // ptr += "%</p>";
  // ptr += "<p>Pressure: ";
  // ptr += pressure;
  // ptr += "hPa</p>";
  // ptr += "<p>Altitude: ";
  // ptr += altitude;
  // ptr += "m</p>";
  // ptr += "</div>\n";
  // ptr += "</body>\n";
  // ptr += "</html>\n";
  // return ptr;
}

void InitFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
}

void GetFile(String fileName)
{
  File file = SPIFFS.open(fileName);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // vector<String> v;
  while (file.available())
  {
    v.push_back(file.readStringUntil('\n'));
  }

  file.close();
}

//------------------------------------------------------

// #include <Arduino.h>
// #include <WiFi.h>

// const char *ssid = "Csepp2";       /* Add your router's SSID */
// const char *password = "Karolyi1"; /*Add the password */

// WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
// /* 80 is the Port Number for HTTP Web Server */

// /* A String to capture the incoming HTTP GET Request */
// String request;

// void setup()
// {
//   // put your setup code here, to run once:
//   Serial.begin(9600);
//   Serial.print("\n");
//   Serial.print("Connecting to: ");
//   Serial.println(ssid);
//   WiFi.mode(WIFI_STA);        /* Configure ESP32 in STA Mode */
//   WiFi.begin(ssid, password); /* Connect to Wi-Fi based on the above SSID and Password */
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     Serial.print("*");
//     delay(100);
//   }
//   Serial.print("\n");
//   Serial.print("Connected to Wi-Fi: ");
//   Serial.println(WiFi.SSID());
//   delay(2000);
//   Serial.print("\n");
//   Serial.println("Starting ESP32 Web Server...");
//   espServer.begin(); /* Start the HTTP web Server */
//   Serial.println("ESP32 Web Server Started");
//   Serial.print("\n");
//   Serial.print("The URL of ESP32 Web Server is: ");
//   Serial.print("http://");
//   Serial.println(WiFi.localIP());
//   Serial.print("\n");
//   Serial.println("Use the above URL in your Browser to access ESP32 Web Server\n");
// }

// void loop()
// {
//   // put your main code here, to run repeatedly:
//   WiFiClient client = espServer.available(); /* Check if a client is available */
//   if (!client)
//   {
//     return;
//   }

//   Serial.println("New Client!!!");
//   boolean currentLineIsBlank = true;
//   while (client.connected())
//   {
//     if (client.available())
//     {
//       char c = client.read();
//       request += c;
//       Serial.write(c);
//       /* if you've gotten to the end of the line (received a newline */
//       /* character) and the line is blank, the http request has ended, */
//       /* so you can send a reply */
//       if (c == '\n' && currentLineIsBlank)
//       {
//         /* Extract the URL of the request */
//         /* We have four URLs. If IP Address is 192.168.1.6 (for example),
//          * then URLs are:
//          * 192.168.1.6/GPIO16ON
//          * 192.168.1.6/GPIO16OFF
//          * 192.168.1.6/GPIO17ON
//          * 192.168.1.6/GPIO17OFF
//          */
//         /* Based on the URL from the request, turn the LEDs ON or OFF */
//         if (request.indexOf("/GPIO16ON") != -1)
//         {
//           Serial.println("GPIO16 LED is ON");
//           // digitalWrite(gpio16LEDPin, HIGH);
//           // gpio16Value = HIGH;
//         }
//         if (request.indexOf("/GPIO16OFF") != -1)
//         {
//           Serial.println("GPIO16 LED is OFF");
//           // digitalWrite(gpio16LEDPin, LOW);
//           // gpio16Value = LOW;
//         }
//         if (request.indexOf("/GPIO17ON") != -1)
//         {
//           Serial.println("GPIO17 LED is ON");
//           // digitalWrite(gpio17LEDPin, HIGH);
//           // gpio17Value = HIGH;
//         }
//         if (request.indexOf("/GPIO17OFF") != -1)
//         {
//           Serial.println("GPIO17 LED is OFF");
//           // digitalWrite(gpio17LEDPin, LOW);
//           // gpio17Value = LOW;
//         }

//         /* HTTP Response in the form of HTML Web Page */
//         client.println("HTTP/1.1 200 OK");
//         client.println("Content-Type: text/html");
//         client.println("Connection: close");
//         client.println(); //  IMPORTANT

//         client.println("<!DOCTYPE HTML>");
//         client.println("<html>");

//         client.println("<head>");
//         client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
//         client.println("<link rel=\"icon\" href=\"data:,\">");

//         client.println("<style>");

//         client.println("html { font-family: Courier New; display: inline-block; margin: 0px auto; text-align: center;}");
//         client.println(".button {border: none; color: white; padding: 10px 20px; text-align: center;");
//         client.println("text-decoration: none; font-size: 25px; margin: 2px; cursor: pointer;}");
//         client.println(".button1 {background-color: #FF0000;}");
//         client.println(".button2 {background-color: #00FF00;}");

//         client.println("</style>");

//         client.println("</head>");

//         client.println("<body>");

//         client.println("<h2>ESP32 Web Server</h2>");

//         // if (gpio16Value == LOW)
//         // {
//         client.println("<p>GPIO16 LED Status: OFF</p>");
//         client.print("<p><a href=\"/GPIO16ON\"><button class=\"button button1\">Click to turn ON</button></a></p>");
//         // }
//         // else
//         // {
//         //   client.println("<p>GPIO16 LED Status: ON</p>");
//         //   client.print("<p><a href=\"/GPIO16OFF\"><button class=\"button button2\">Click to turn OFF</button></a></p>");
//         // }

//         // if (gpio17Value == LOW)
//         // {
//         client.println("<p>GPIO17 LED Status: OFF</p>");
//         client.print("<p><a href=\"/GPIO17ON\"><button class=\"button button1\">Click to turn ON</button></a></p>");
//         // }
//         // else
//         // {
//         //   client.println("<p>GPIO17 LED Status: ON</p>");
//         //   client.print("<p><a href=\"/GPIO17OFF\"><button class=\"button button2\">Click to turn OFF</button></a></p>");
//         // }

//         client.println("</body>");

//         client.println("</html>");

//         break;
//       }
//       if (c == '\n')
//       {
//         currentLineIsBlank = true;
//       }
//       else if (c != '\r')
//       {
//         currentLineIsBlank = false;
//       }
//       // client.print("\n");
//     }
//   }

//   delay(1);
//   request = "";
//   // client.flush();
//   client.stop();
//   Serial.println("Client disconnected");
//   Serial.print("\n");
// }

//------------------------------------------------------

// #include <WiFi.h>
// #include <WebServer.h>

// /* Put your SSID & Password */
// const char* ssid = "ESP32";  // Enter SSID here
// const char* password = "12345678";  //Enter Password here

// /* Put IP Address details */
// IPAddress local_ip(192,168,1,1);
// IPAddress gateway(192,168,1,1);
// IPAddress subnet(255,255,255,0);

// WebServer server(80);

// uint8_t LED1pin = 4;
// bool LED1status = LOW;

// uint8_t LED2pin = 5;
// bool LED2status = LOW;

// void setup() {
//   Serial.begin(115200);
//   pinMode(LED1pin, OUTPUT);
//   pinMode(LED2pin, OUTPUT);

//   WiFi.softAP(ssid, password);
//   WiFi.softAPConfig(local_ip, gateway, subnet);
//   delay(100);

//   server.on("/", handle_OnConnect);
//   server.on("/led1on", handle_led1on);
//   server.on("/led1off", handle_led1off);
//   server.on("/led2on", handle_led2on);
//   server.on("/led2off", handle_led2off);
//   server.onNotFound(handle_NotFound);

//   server.begin();
//   Serial.println("HTTP server started");
// }
// void loop() {
//   server.handleClient();
//   if(LED1status)
//   {digitalWrite(LED1pin, HIGH);}
//   else
//   {digitalWrite(LED1pin, LOW);}

//   if(LED2status)
//   {digitalWrite(LED2pin, HIGH);}
//   else
//   {digitalWrite(LED2pin, LOW);}
// }

// void handle_OnConnect() {
//   LED1status = LOW;
//   LED2status = LOW;
//   Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
//   server.send(200, "text/html", SendHTML(LED1status,LED2status));
// }

// void handle_led1on() {
//   LED1status = HIGH;
//   Serial.println("GPIO4 Status: ON");
//   server.send(200, "text/html", SendHTML(true,LED2status));
// }

// void handle_led1off() {
//   LED1status = LOW;
//   Serial.println("GPIO4 Status: OFF");
//   server.send(200, "text/html", SendHTML(false,LED2status));
// }

// void handle_led2on() {
//   LED2status = HIGH;
//   Serial.println("GPIO5 Status: ON");
//   server.send(200, "text/html", SendHTML(LED1status,true));
// }

// void handle_led2off() {
//   LED2status = LOW;
//   Serial.println("GPIO5 Status: OFF");
//   server.send(200, "text/html", SendHTML(LED1status,false));
// }

// void handle_NotFound(){
//   server.send(404, "text/plain", "Not found");
// }

// String SendHTML(uint8_t led1stat,uint8_t led2stat){
//   String ptr = "<!DOCTYPE html> <html>\n";
//   ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
//   ptr +="<title>LED Control</title>\n";
//   ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
//   ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
//   ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
//   ptr +=".button-on {background-color: #3498db;}\n";
//   ptr +=".button-on:active {background-color: #2980b9;}\n";
//   ptr +=".button-off {background-color: #34495e;}\n";
//   ptr +=".button-off:active {background-color: #2c3e50;}\n";
//   ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
//   ptr +="</style>\n";
//   ptr +="</head>\n";
//   ptr +="<body>\n";
//   ptr +="<h1>ESP32 Web Server</h1>\n";
//   ptr +="<h3>Using Access Point(AP) Mode</h3>\n";

//    if(led1stat)
//   {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
//   else
//   {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

//   if(led2stat)
//   {ptr +="<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";}
//   else
//   {ptr +="<p>LED2 Status: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";}

//   ptr +="</body>\n";
//   ptr +="</html>\n";
//   return ptr;
// }
