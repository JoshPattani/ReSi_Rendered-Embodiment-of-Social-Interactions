#include <Arduino.h>
#include <stdint.h>
#include <math.h> // For sqrt()
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
#define SECRET_IP IPAddress(192, 168, 0, 8) // Target IP address (where your Max/MSP is running)
#define SECRET_PORT 42069                   // Target port. Must match the port in your Max/MSP patch.

// WiFi credentials
const char *ssid = "UCB Wireless"; // Your WiFi SSID

// Your university login credentials
const char *identikey = SECRET_IDENTIKEY; // Your university identikey
const char *password = SECRET_PASS;       // Your university password

// Variables to store portal information
String captivePortalURL = "";
String authURL = "";
String authToken = "";

// Mode selection flags:
// User identifier (for multi-participant synchronization) Used for tagging data from different users in multi-user scenarios.
const String USER_A = "A";  // Participant A
const String USER_B = "B";  // Participant B
const String USER = USER_A; // Current participant identifier

// Serial communication parameters
const int BAUDRATE = 115200;

// Debug flag
const bool DEBUG = true;

//   AnalogMode = true -> analog (oscilloscope) mode; false -> digital interval mode.
//   CHANNEL: false selects channel A; true selects channel B.
bool AnalogMode = false; // set to false for digital interval mode (required for HRV)

// --------------------------------------------- //

// =============================== //
// ****** MCU Configuration ****** //
// =============================== //

// Analog pin for PPG sensor input:
const int analogPin = A0; // Analog input pin for heart rate sensor
const int digitalPin = 2; // Digital input pin for heart rate sensor
const int sensorPin = digitalPin;

// --------------------------------------------- //

// =============================== //
// ****** HRV Data Storage ******* //
// =============================== //

// --- HRV data storage (digital mode) ---
// We assume a 2.5‑minute window. At high heart rates, you might see up to 300 beats.
// For simplicity, we use a fixed-size buffer. Modify as needed.
#define HRV_BUFFER_SIZE 300
#define HRV_WINDOW_US 150000000UL // 2.5 minutes in microseconds

// HR metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
float hRCurrentValue; // Current heart rate
float hRUserMax;      // User-defined maximum heart rate
float hRUserMin;      // User-defined minimum heart rate

// HRV metrics
// Declared in arduino properties. Uncomment if not using IoT Cloud
float rmssd; // Root mean square of successive differences
float sdann; // Standard deviation of the average NN intervals
float sdnn;  // Standard deviation of NN intervals

// HRV buffer and index
unsigned long NN_intervals[HRV_BUFFER_SIZE]; // Buffer to store NN intervals
volatile unsigned long NN_index = 0;
volatile unsigned long hrvWindowStart = 0;

// --- Heart Rate Variables ---
unsigned long lastBeatTime = 0;

// Hrv reset function
void resetHRV()
{
  hRCurrentValue = 0;
  hRUserMax = -1000000;
  hRUserMin = 1000000;

  // Reset HRV metrics
  rmssd = 0;
  sdann = 0;
  sdnn = 0;

  for (unsigned long i = 0; i < HRV_BUFFER_SIZE; i++)
  {
    NN_intervals[i] = 0;
  }
  NN_index = 0;
  hrvWindowStart = micros();
}

// --------------------------------------------- //

// =============================== //
// ****** Heart Rate ****** //
// =============================== //
volatile unsigned long lastPulseTime = 0;  // Stores the time of the last beat
volatile unsigned long pulseIntervals[10]; // Stores last 10 pulse intervals
volatile uint8_t pulseIndex = 0;           // Rolling index for interval storage
volatile bool newPulse = false;            // Flag for pulse detection
bool calibrated = false;                   // Flag for user calibration

// Calculate rolling average BPM
int calculateBPM()
{
  unsigned long sum = 0;
  for (int i = 0; i < 10; i++)
  {
    sum += pulseIntervals[i];
  }
  unsigned long avgInterval = sum / 10;

  if (avgInterval > 0)
  {
    return 60000 / avgInterval; // Convert ms to BPM
  }

  return avgInterval;
}

// Interrupt Service Routine (ISR) for detecting heartbeats
void pulseISR()
{
  unsigned long currentTime = millis();
  unsigned long interval = currentTime - lastPulseTime;

  if (interval > 300 && interval < 2000)
  { // Ignore noise (too fast/slow beats)
    pulseIntervals[pulseIndex] = interval;
    pulseIndex = (pulseIndex + 1) % 10; // Move to next slot in rolling average array

    // Store the interval in the NN_intervals buffer for HRV analysis
    // (For simplicity, if the buffer fills, we wrap around.)
    if (NN_index < HRV_BUFFER_SIZE)
    {
      NN_intervals[NN_index++] = interval;
    }
    else
    {
      // Overwrite oldest data (circular buffer)
      NN_intervals[NN_index % HRV_BUFFER_SIZE] = interval;
      NN_index++;
    }

    newPulse = true; // Notify main loop
  }

  lastPulseTime = currentTime;
}

// --------------------------------------------- //

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// OSC message addressing
// "/pulse" -> Pulse detection message
// "/hearRate" -> Heart rate message
// "/rmssd" -> Root mean square of successive differences
// "/sdann" -> Standard deviation of the average NN intervals
// "/sdnn" -> Standard deviation of NN intervals

// UDP connection
WiFiUDP udp;
bool UDPConnected = false;

// Update intervals
const unsigned long oscSendInterval = 50;      // 50ms = 20Hz send rate
const unsigned long cloudSendInterval = 1000;  // 1000ms = 1Hz cloud update rate
const unsigned long minMaxSendInterval = 1000; // 1000ms = 1Hz send rate

// Timers
unsigned long lastOscSendTime = 0;
unsigned long lastCloudSendTime = 0;
unsigned long lastMinMaxSendTime = 0; // timer for min/max updates

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

// =============================== //
// ****** UDP Connection ****** //
// =============================== //

// Udp Client handling
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  udp.begin(SECRET_PORT); // Local port to listen on (doesn't matter much for sending)
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

// ============================== //
// ****** HRV Computation ******* //
// ============================== //

// --- HRV Computation ---
// This function computes SDNN, RMSSD, and SDANN from the NN_intervals collected in the current window.
void computeHRVMetrics()
{
  // First, disable interrupts while we copy the buffer.
  noInterrupts();
  int n = NN_index;
  // Create a local copy (in milliseconds) in an array.
  double intervals_ms[HRV_BUFFER_SIZE];
  for (int i = 0; i < n && i < HRV_BUFFER_SIZE; i++)
  {
    intervals_ms[i] = NN_intervals[i] * 0.001; // convert µs to ms
  }
  // Reset the global buffer for the next window.
  NN_index = 0;
  // Reset the window start time.
  hrvWindowStart = micros();
  interrupts();

  if (n < 2)
  {
    Serial.println("Not enough data for HRV computation.");
    return;
  }

  // Compute the mean of the NN intervals.
  double sum = 0;
  for (int i = 0; i < n; i++)
  {
    sum += intervals_ms[i];
  }
  double mean = sum / n;

  // Compute SDNN: standard deviation of all NN intervals.
  double sdnn_sum = 0;
  for (int i = 0; i < n; i++)
  {
    double diff = intervals_ms[i] - mean;
    sdnn_sum += diff * diff;
  }
  double sdnn = sqrt(sdnn_sum / n);

  // Compute RMSSD: square root of the mean squared differences of successive NN intervals.
  double rmssd_sum = 0;
  int count_diff = 0;
  for (int i = 0; i < n - 1; i++)
  {
    double diff = intervals_ms[i + 1] - intervals_ms[i];
    rmssd_sum += diff * diff;
    count_diff++;
  }
  double rmssd = (count_diff > 0) ? sqrt(rmssd_sum / count_diff) : 0;

  // Compute SDANN: approximate by dividing the window into 5 equal segments.
  int segments = 5;
  if (n < segments)
  {
    Serial.println("Not enough data for SDANN computation.");
    return;
  }
  int segSize = n / segments;
  double segMeans[5];
  for (int s = 0; s < segments; s++)
  {
    double segSum = 0;
    for (int i = s * segSize; i < (s + 1) * segSize; i++)
    {
      segSum += intervals_ms[i];
    }
    segMeans[s] = segSum / segSize;
  }
  // Compute standard deviation of the segment means.
  double segMeanSum = 0;
  for (int s = 0; s < segments; s++)
  {
    segMeanSum += segMeans[s];
  }
  double segMeanAvg = segMeanSum / segments;
  double sdann_sum = 0;
  for (int s = 0; s < segments; s++)
  {
    double diff = segMeans[s] - segMeanAvg;
    sdann_sum += diff * diff;
  }
  double sdann = sqrt(sdann_sum / segments);

  if (DEBUG)
  {
    // Print the computed HRV metrics.
    Serial.print("HRV Metrics: SDNN = ");
    Serial.print(sdnn);
    Serial.print(" ms, RMSSD = ");
    Serial.print(rmssd);
    Serial.print(" ms, SDANN = ");
    Serial.print(sdann);
    Serial.println(" ms");
  }
}

// --------------------------------------------- //

// =============================== //
// ****** Setup and Loop ******* //
// =============================== //

void setup()
{
  Serial.begin(BAUDRATE);
  delay(1500);

  // Initialize PPG sensor
  // For HRV, we use the digital pin for interval measurement.
  if (!AnalogMode)
  {
    pinMode(sensorPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensorPin), pulseISR, RISING);
    // Initialize the last beat time.
    lastBeatTime = micros();
    // Set the HRV window start time.
    hrvWindowStart = micros();

    // Initialize HRV metrics.
    resetHRV();

    // Initialize array with reasonable values (assume ~60 BPM at start)
    for (int i = 0; i < 10; i++)
    {
      pulseIntervals[i] = 1000;
    }
  }
  else
  {
    // Analog mode: set up analog pin
    pinMode(analogPin, INPUT);
  }

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
  // Beat intervals are captured in the ISR.
  // Check if the current window (2.5 minutes) has elapsed.
  unsigned long now = micros();
  if (!AnalogMode && (now - hrvWindowStart) >= HRV_WINDOW_US)
  {
    // Compute and output HRV metrics for the current window.
    computeHRVMetrics();
    // (The computeHRVMetrics() function resets the window start and clears the buffer.)

    // Send the HRV metrics through OSC.
    sendOSCMessage("/rmssd", rmssd);
    sendOSCMessage("/sdann", sdann);
    sendOSCMessage("/sdnn", sdnn);
  }
  if (newPulse)
  {
    newPulse = false;

    // Sending pulse message with static value indicating pulse detection
    sendOSCMessage("/pulse", 1);

    // Update the cloud variable with the latest BPM value.
    hRCurrentValue = calculateBPM(); // Assuming calculateBPM() is a function that computes the latest BPM

    if (hRCurrentValue > 0)
    {
      Serial.print("Heart Rate: ");
      Serial.println(hRCurrentValue);

      if (!calibrated)
      {
        hRUserMin = hRCurrentValue;
        hRUserMax = hRCurrentValue;
        calibrated = true;
      }
      else
      {
        if (hRUserMax < hRCurrentValue)
          hRUserMax = hRCurrentValue;
        if (hRUserMin > hRCurrentValue)
          hRUserMin = hRCurrentValue;
      }
    }
  }

  // Dynamic min/max updates bases on change in values
  if (millis() - lastMinMaxSendTime >= minMaxSendInterval)
  {
    lastMinMaxSendTime = millis();
    if (hRCurrentValue > 0)
    {
      sendOSCMessage("/minHeartRate", hRUserMin);
      sendOSCMessage("/maxHeartRate", hRUserMax);
    }
  }

  unsigned long currentTime = millis();

  // Send OSC messages at specified interval
  if (currentTime - lastOscSendTime >= oscSendInterval)
  {
    lastOscSendTime = currentTime;
    sendOSCMessage("/heartRate", hRCurrentValue);
  }
}