/*
 * Basic Captive Portal WiFi connection with OSC Sender for Arduino Nano ESP32

  * This sketch connects to a WiFi network, handles captive portal authentication, and sends OSC messages.
 */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <OSCMessage.h>
#include "arduino_secrets.h"

// ================================ //
// ***** Configuration Params ***** //
// ================================ //

// Secrets for OSC communication
#define SECRET_IP IPAddress(192, 168, 0, 8) // Target IP address
#define SECRET_PORT 12345                   // Target port. Must match the port in your OSC application.
#define SECRET_LOCAL_PORT 12346             // Local port to listen for OSC messages
#define SECRET_LOCAL_PORT_DISCOVERY 12347   // Local port for OSC discovery messages

// WiFi credentials
const char *ssid = "UCB Wireless"; // Your WiFi SSID

// Your university login credentials
const char *identikey = SECRET_IDENTIKEY; // Your identikey
const char *password = SECRET_PASS;       // Your login password

// Variables to store portal information
String captivePortalURL = "";
String authURL = "";
String authToken = "";

// Debug flag
const bool DEBUG = true;

// --------------------------------------------- //

// =============================== //
// ****** MCU Configuration ****** //
// =============================== //

// Pin configuration
const int analogPin = A0; // Analog input pin
const int digitalPin = 2; // Digital input pin
const int sensorPin = digitalPin;

// Serial communication parameters
const int BAUDRATE = 115200;

bool newEvent = false; // Flag to indicate a new event (e.g., pulse detected)
// --------------------------------------------- //

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// Update intervals
const unsigned long oscSendInterval = 50; // 50ms = 20Hz send rate

// Timers
unsigned long lastOscSendTime = 0;

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Begin UDP packet
  udp.beginPacket(SECRET_IP, SECRET_PORT);

  // Write OSC message to UDP
  msg.send(udp);

  // End packet and send
  udp.endPacket();

  // Free space
  msg.empty();

  if (DEBUG)
  {
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
  }
}

// --------------------------------------------- //

// ================================ //
// ******** UDP Connection ******** //
// ================================ //

// UDP connection
WiFiUDP udp;
bool UDPConnected = false;

// Udp Client handling
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  udp.begin(SECRET_LOCAL_PORT_DISCOVERY); // Local port to listen on (only matters for IP discovery, not much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// --------------------------------------------- //

// =============================== //
// ****** WiFi Connection ****** //
// =============================== //

String extractLoginURL(String html)
{
  // This is a simplified extraction that looks for common patterns
  // You may need to customize this for your specific captive portal

  // Look for forms with post action
  int formIndex = html.indexOf("<form");
  if (formIndex > 0)
  {
    int actionIndex = html.indexOf("action=", formIndex);
    if (actionIndex > 0)
    {
      int quoteChar = html.charAt(actionIndex + 7); // Either ' or "
      int urlStart = actionIndex + 8;
      int urlEnd = html.indexOf(quoteChar, urlStart);

      String formAction = html.substring(urlStart, urlEnd);

      // If it's a relative URL, we need to prepend the base URL
      if (formAction.startsWith("/"))
      {
        // Extract base URL from the current page
        String baseURL = "";
        int httpIndex = html.indexOf("http");
        if (httpIndex >= 0)
        {
          int slashAfterDomain = html.indexOf("/", httpIndex + 8);
          if (slashAfterDomain > 0)
          {
            baseURL = html.substring(httpIndex, slashAfterDomain);
          }
        }

        return baseURL + formAction;
      }

      return formAction;
    }
  }

  // If we couldn't find a form, look for redirect URLs
  int redirectIndex = html.indexOf("window.location");
  if (redirectIndex > 0)
  {
    int urlStart = html.indexOf("'", redirectIndex);
    if (urlStart < 0)
    {
      urlStart = html.indexOf("\"", redirectIndex);
    }

    if (urlStart > 0)
    {
      int quoteChar = html.charAt(urlStart);
      urlStart++;
      int urlEnd = html.indexOf(quoteChar, urlStart);

      return html.substring(urlStart, urlEnd);
    }
  }

  // Extract auth token if present
  int tokenIndex = html.indexOf("name=\"auth_token\"");
  if (tokenIndex > 0)
  {
    int valueIndex = html.indexOf("value=", tokenIndex);
    if (valueIndex > 0)
    {
      int quoteChar = html.charAt(valueIndex + 6); // Either ' or "
      int tokenStart = valueIndex + 7;
      int tokenEnd = html.indexOf(quoteChar, tokenStart);

      authToken = html.substring(tokenStart, tokenEnd);
      Serial.print("Found auth token: ");
      Serial.println(authToken);
    }
  }

  return ""; // Couldn't find a login URL
}

bool detectCaptivePortal()
{
  HTTPClient http;

  // Try to connect to a known website that should redirect to the captive portal
  // Apple's captive portal detection URL
  http.begin("http://captive.apple.com/hotspot-detect.html");

  int httpCode = http.GET();
  Serial.print("HTTP GET status code: ");
  Serial.println(httpCode);

  if (httpCode > 0)
  {
    String payload = http.getString();
    http.end();

    // Check if we got redirected (captive portal will usually return 302)
    if (httpCode == 302 || payload.indexOf("login") > 0 || payload.indexOf("auth") > 0)
    {
      // Try to extract the login URL from the response
      captivePortalURL = extractLoginURL(payload);

      if (captivePortalURL.length() > 0)
      {
        Serial.print("Detected captive portal URL: ");
        Serial.println(captivePortalURL);
        return true;
      }
    }
  }

  http.end();

  // Alternative method: Try to detect by checking for DNS hijacking
  http.begin("http://example.com");
  httpCode = http.GET();

  if (httpCode > 0)
  {
    String payload = http.getString();
    http.end();

    // If we don't get the expected response from example.com, we're likely in a captive portal
    if (payload.indexOf("login") > 0 || payload.indexOf("auth") > 0 || payload.indexOf("captive") > 0)
    {
      captivePortalURL = extractLoginURL(payload);

      if (captivePortalURL.length() > 0)
      {
        Serial.print("Detected captive portal URL (alt method): ");
        Serial.println(captivePortalURL);
        return true;
      }
    }
  }

  http.end();
  return false;
}

// URL encoding function
String urlEncode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;

  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);

    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isAlphaNumeric(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
        code1 = (c & 0xf) - 10 + 'A';
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
        code0 = c - 10 + 'A';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }

  return encodedString;
}

bool authenticateCaptivePortal()
{
  if (captivePortalURL.length() == 0)
  {
    Serial.println("No captive portal URL detected!");
    return false;
  }

  HTTPClient http;
  http.begin(captivePortalURL);

  // First, GET the login page to extract any necessary tokens or cookies
  int httpCode = http.GET();
  Serial.print("Login page GET status: ");
  Serial.println(httpCode);

  if (httpCode > 0)
  {
    String loginPage = http.getString();

    // Extract auth token if not already done
    if (authToken.length() == 0)
    {
      int tokenIndex = loginPage.indexOf("name=\"auth_token\"");
      if (tokenIndex > 0)
      {
        int valueIndex = loginPage.indexOf("value=", tokenIndex);
        if (valueIndex > 0)
        {
          int quoteChar = loginPage.charAt(valueIndex + 6); // Either ' or "
          int tokenStart = valueIndex + 7;
          int tokenEnd = loginPage.indexOf(quoteChar, tokenStart);

          authToken = loginPage.substring(tokenStart, tokenEnd);
          Serial.print("Found auth token: ");
          Serial.println(authToken);
        }
      }
    }

    // Extract the form action URL if different from captive portal URL
    int formIndex = loginPage.indexOf("<form");
    if (formIndex > 0)
    {
      int actionIndex = loginPage.indexOf("action=", formIndex);
      if (actionIndex > 0)
      {
        int quoteChar = loginPage.charAt(actionIndex + 7); // Either ' or "
        int urlStart = actionIndex + 8;
        int urlEnd = loginPage.indexOf(quoteChar, urlStart);

        String formAction = loginPage.substring(urlStart, urlEnd);

        // If it's a relative URL, we need to prepend the base URL
        if (formAction.startsWith("/"))
        {
          // Extract domain from the captive portal URL
          int protocolEnd = captivePortalURL.indexOf("://");
          int pathStart = captivePortalURL.indexOf("/", protocolEnd + 3);
          String domain = "";

          if (pathStart > 0)
          {
            domain = captivePortalURL.substring(0, pathStart);
          }
          else
          {
            domain = captivePortalURL;
          }

          authURL = domain + formAction;
        }
        else
        {
          authURL = formAction;
        }
      }
    }

    if (authURL.length() == 0)
    {
      // If we couldn't extract a specific form action, use the captive portal URL
      authURL = captivePortalURL;
    }

    Serial.print("Auth URL: ");
    Serial.println(authURL);

    // Now submit the login form
    http.end();
    http.begin(authURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Prepare POST data - adjust field names as needed for your captive portal
    String postData = "username=" + urlEncode(identikey) +
                      "&password=" + urlEncode(password);

    // Add auth token if found
    if (authToken.length() > 0)
    {
      postData += "&auth_token=" + urlEncode(authToken);
    }

    Serial.println("Sending authentication request...");
    httpCode = http.POST(postData);

    Serial.print("Auth POST status: ");
    Serial.println(httpCode);

    if (httpCode > 0)
    {
      String response = http.getString();

      // Check if authentication was successful
      if (httpCode == 200 || httpCode == 302)
      {
        // Wait a moment for the connection to stabilize
        delay(2000);

        // Check if we're now connected
        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("Successfully authenticated to WiFi!");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          http.end();
          return true;
        }
      }

      // If we're here, authentication might have failed
      Serial.println("Authentication may have failed. Response excerpt:");
      Serial.println(response.substring(0, 200) + "..."); // Print first 200 chars
    }
  }

  http.end();
  return false;
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid);

  // Wait for connection attempt
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    connectLocalPort();
    return;
  }

  // If we're here, we're likely facing a captive portal
  Serial.println("\nFacing captive portal. Attempting to authenticate...");

  // Detect captive portal
  if (detectCaptivePortal())
  {
    // Authenticate through the captive portal
    authenticateCaptivePortal();
  }
  else
  {
    Serial.println("Failed to detect captive portal. Check WiFi credentials.");
  }
}

// =============================== //
// ****** Setup and Loop ******* //
// =============================== //

void setup()
{
  Serial.begin(BAUDRATE);
  delay(1500);

  pinMode(sensorPin, INPUT);

  // Your setup code here

  connectToWiFi();
}

void loop()
{
  // Periodically check if we're still connected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();

    return;
  }

  // Your main loop code here
  // For example, read sensor data and send it via OSC

  if (newEvent)
  {
    newEvent = false;

    // Sending event message with static value indicating event detection
    sendOSCMessage("/event", 1);
  }

  unsigned long currentTime = millis();

  // Send OSC messages at specified interval
  if (currentTime - lastOscSendTime >= oscSendInterval)
  {
    lastOscSendTime = currentTime;
    sendOSCMessage("/time", currentTime / 1000.0); // Send time in seconds
  }
}