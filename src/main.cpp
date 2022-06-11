#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "Csepp2";       /* Add your router's SSID */
const char *password = "Karolyi1"; /*Add the password */

WiFiServer espServer(80); /* Instance of WiFiServer with port number 80 */
/* 80 is the Port Number for HTTP Web Server */

/* A String to capture the incoming HTTP GET Request */
String request;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.print("\n");
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);        /* Configure ESP32 in STA Mode */
  WiFi.begin(ssid, password); /* Connect to Wi-Fi based on the above SSID and Password */
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("*");
    delay(100);
  }
  Serial.print("\n");
  Serial.print("Connected to Wi-Fi: ");
  Serial.println(WiFi.SSID());
  delay(2000);
  Serial.print("\n");
  Serial.println("Starting ESP32 Web Server...");
  espServer.begin(); /* Start the HTTP web Server */
  Serial.println("ESP32 Web Server Started");
  Serial.print("\n");
  Serial.print("The URL of ESP32 Web Server is: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.print("\n");
  Serial.println("Use the above URL in your Browser to access ESP32 Web Server\n");
}

void loop()
{
  // put your main code here, to run repeatedly:
  WiFiClient client = espServer.available(); /* Check if a client is available */
  if (!client)
  {
    return;
  }

  Serial.println("New Client!!!");
  boolean currentLineIsBlank = true;
  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      request += c;
      Serial.write(c);
      /* if you've gotten to the end of the line (received a newline */
      /* character) and the line is blank, the http request has ended, */
      /* so you can send a reply */
      if (c == '\n' && currentLineIsBlank)
      {
        /* Extract the URL of the request */
        /* We have four URLs. If IP Address is 192.168.1.6 (for example),
         * then URLs are:
         * 192.168.1.6/GPIO16ON
         * 192.168.1.6/GPIO16OFF
         * 192.168.1.6/GPIO17ON
         * 192.168.1.6/GPIO17OFF
         */
        /* Based on the URL from the request, turn the LEDs ON or OFF */
        if (request.indexOf("/GPIO16ON") != -1)
        {
          Serial.println("GPIO16 LED is ON");
          // digitalWrite(gpio16LEDPin, HIGH);
          // gpio16Value = HIGH;
        }
        if (request.indexOf("/GPIO16OFF") != -1)
        {
          Serial.println("GPIO16 LED is OFF");
          // digitalWrite(gpio16LEDPin, LOW);
          // gpio16Value = LOW;
        }
        if (request.indexOf("/GPIO17ON") != -1)
        {
          Serial.println("GPIO17 LED is ON");
          // digitalWrite(gpio17LEDPin, HIGH);
          // gpio17Value = HIGH;
        }
        if (request.indexOf("/GPIO17OFF") != -1)
        {
          Serial.println("GPIO17 LED is OFF");
          // digitalWrite(gpio17LEDPin, LOW);
          // gpio17Value = LOW;
        }

        /* HTTP Response in the form of HTML Web Page */
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println(); //  IMPORTANT

        client.println("<!DOCTYPE HTML>");
        client.println("<html>");

        client.println("<head>");
        client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
        client.println("<link rel=\"icon\" href=\"data:,\">");

        client.println("<style>");

        client.println("html { font-family: Courier New; display: inline-block; margin: 0px auto; text-align: center;}");
        client.println(".button {border: none; color: white; padding: 10px 20px; text-align: center;");
        client.println("text-decoration: none; font-size: 25px; margin: 2px; cursor: pointer;}");
        client.println(".button1 {background-color: #FF0000;}");
        client.println(".button2 {background-color: #00FF00;}");

        client.println("</style>");

        client.println("</head>");

        client.println("<body>");

        client.println("<h2>ESP32 Web Server</h2>");

        // if (gpio16Value == LOW)
        // {
        client.println("<p>GPIO16 LED Status: OFF</p>");
        client.print("<p><a href=\"/GPIO16ON\"><button class=\"button button1\">Click to turn ON</button></a></p>");
        // }
        // else
        // {
        //   client.println("<p>GPIO16 LED Status: ON</p>");
        //   client.print("<p><a href=\"/GPIO16OFF\"><button class=\"button button2\">Click to turn OFF</button></a></p>");
        // }

        // if (gpio17Value == LOW)
        // {
        client.println("<p>GPIO17 LED Status: OFF</p>");
        client.print("<p><a href=\"/GPIO17ON\"><button class=\"button button1\">Click to turn ON</button></a></p>");
        // }
        // else
        // {
        //   client.println("<p>GPIO17 LED Status: ON</p>");
        //   client.print("<p><a href=\"/GPIO17OFF\"><button class=\"button button2\">Click to turn OFF</button></a></p>");
        // }

        client.println("</body>");

        client.println("</html>");

        break;
      }
      if (c == '\n')
      {
        currentLineIsBlank = true;
      }
      else if (c != '\r')
      {
        currentLineIsBlank = false;
      }
      // client.print("\n");
    }
  }

  delay(1);
  request = "";
  // client.flush();
  client.stop();
  Serial.println("Client disconnected");
  Serial.print("\n");
}

String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 Web Server</h1>\n";
  ptr +="<h3>Using Access Point(AP) Mode</h3>\n";
  
   if(led1stat)
  {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
  else
  {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

  if(led2stat)
  {ptr +="<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";}
  else
  {ptr +="<p>LED2 Status: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}