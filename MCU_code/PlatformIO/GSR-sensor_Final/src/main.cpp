/*
  GSR Sensor with OSC Output
  This code reads a GSR sensor and sends the data to a computer over WiFi using OSC.
  The code is designed to work with the Arduino Nano ESP32 microcontroller and PlatformIO.

  The code is part of the RESI project at CU Boulder, developed by Josh Pattani.
  For more information, visit: https://www.resiforevery.one

  ✌️ Cheers!
 */
#include <Arduino.h>
#include "networkTypes.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "wifiHandler.h"
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include "wifi_stability.h"

// Network configuration
WiFiUDP Udp;                           // Create a UDP instance for OSC communication
bool UDPConnected = false;             // Flag to check if UDP connection is established
extern NetworkType currentNetworkType; // Reference the type from wifiHandler.h

// IP address variables
IPAddress discoveredAudioIP;  // Will store dynamically discovered audio computer IP
IPAddress discoveredVisualIP; // Will store dynamically discovered visual computer IP
bool audioIPDiscovered = false;
bool visualIPDiscovered = false;

// Multiple IP and port targets for OSC communication
const unsigned int inPort = SECRET_LOCAL_PORT;         // Port to listen for OSC messages
const IPAddress outIpAudio(SECRET_AUDIO_IP);           // Audio computer. Defined in arduino_secrets.h
const unsigned int outPortAudio = SECRET_AUDIO_PORT;   // Target port. Must match the port in Max
const IPAddress outIpVisual(SECRET_VISUAL_IP);         // Visual computer
const unsigned int outPortVisual = SECRET_VISUAL_PORT; // Target port. Must match the port in Max

// Function prototypes
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void sendAcknowledgment(IPAddress targetIP, unsigned int targetPort, const char *type, const char *user);
void listenForDiscovery();

// Debug flag
const bool DEBUG = true;

String user = "/User_" + String(USER); // User identifier (A or B)

// GSR sensor variables
int gSRCurrentValue;
int gSRUserMin;
int gSRUserMax;

const int sweatSensorPin = A0;
const int sampleSize = 5; // Reduce the number of samples for less smoothing (try 2 or 3 as well)
bool calibrated = false;

int readings[sampleSize]; // Array to hold sensor readings for moving average
int readIndex = 0;        // Current index for the moving average
int total = 0;            // Running total for the moving average
int average = 0;          // Average value

// Send acknowledgment back to the discovered computer
void sendAcknowledgment(IPAddress targetIP, unsigned int targetPort, const char *type, const char *user)
{
  // small delay to ensure the packet is sent correctly
  delay(10);

  OSCMessage msg("/arduino/acknowledgment");
  msg.add(type);
  msg.add(user);

  Udp.beginPacket(targetIP, targetPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  // Add a second acknowledgment for redundancy
  delay(100);
  Udp.beginPacket(targetIP, targetPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  if (DEBUG)
  {
    Serial.print("Sent acknowledgment to ");
    Serial.print(type);
    Serial.print(" computer at IP: ");
    Serial.println(targetIP);
  }
}

void listenForDiscovery()
{
  // Check if discovery messages are available
  int packetSize = Udp.parsePacket();

  if (packetSize)
  {
    // If we already have this IP discovered, ignore the packet
    if ((audioIPDiscovered && Udp.remoteIP() == discoveredAudioIP) ||
        (visualIPDiscovered && Udp.remoteIP() == discoveredVisualIP))
    {
      // Clear buffer but don't process
      char packetBuffer[255];
      Udp.read(packetBuffer, packetSize);
      return;
    }

    // Read the packet into a buffer
    char packetBuffer[255];
    Udp.read(packetBuffer, packetSize);
    packetBuffer[packetSize] = 0; // Null-terminate the string

    // Process the OSC message
    OSCMessage msg;
    for (int i = 0; i < packetSize; i++)
    {
      msg.fill(packetBuffer[i]);
    }

    if (msg.fullMatch("/audioComputer/discover"))
    {
      // Extract sender's IP address
      discoveredAudioIP = Udp.remoteIP();
      audioIPDiscovered = true;

      // Send acknowledgment
      sendAcknowledgment(discoveredAudioIP, outPortAudio, "audio", user.c_str());

      if (DEBUG)
      {
        Serial.print("Received discovery message from audio computer at IP: ");
        Serial.println(discoveredAudioIP);
      }
    }
    else if (msg.fullMatch("/visualComputer/discover"))
    {
      // Extract sender's IP address
      discoveredVisualIP = Udp.remoteIP();
      visualIPDiscovered = true;

      // Send acknowledgment
      sendAcknowledgment(discoveredVisualIP, outPortVisual, "visual", user.c_str());

      if (DEBUG)
      {
        Serial.print("Received discovery message from visual computer at IP: ");
        Serial.println(discoveredVisualIP);
      }
    }
  }
}

void setup()
{
  // Initialize serial and wait for port to open
  Serial.begin(9600);
  delay(4000); // Give more time for serial to initialize

  Serial.println("Starting RESI WiFi detection and connection...");

  // Configure device for stability
  WiFiStabilityManager::configureDevice();

  // First detect network type
  currentNetworkType = detectNetworkType();

  // Connect to the appropriate network based on type
  bool wifiConnected = false;

  if (currentNetworkType == NETWORK_HOME)
  {
    Serial.println("Home network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_STUDIO)
  {
    Serial.println("Studio network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
  }
  else if (currentNetworkType == NETWORK_CAMPUS)
  {
    Serial.println("Campus network detected");
    // Try campus connection (this will handle authentication too)
    if (handleUCBWirelessAuth())
    {
      Serial.println("Successfully connected to campus network");
      wifiConnected = true;

      // Ensure we're stable before proceeding
      WiFiStabilityManager::waitForStableConnection();
    }
    else
    {
      Serial.println("Failed to connect to campus network");
    }
  }
  else
  {
    Serial.println("Unknown network environment");
    // Try home network as default
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }

  // Check final connection status
  Serial.print("Final WiFi status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Initialize UDP for OSC communications
  if (WiFi.status() == WL_CONNECTED)
  {
    connectLocalPort();
  }

  Serial.println("Initializing GSR sensor with OSC output...");

  gSRCurrentValue = 0;
  gSRUserMin = 256; // Set a reasonable default min value
  gSRUserMax = 512; // Set a reasonable default max value

  // Initialize GSR sensor readings
  for (int i = 0; i < sampleSize; i++)
  {
    readings[i] = 0;
  }

  // Check final connection status
  Serial.print("Final WiFi status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());
  }
}

void loop()
{
  // Listen for discovery messages from Max
  listenForDiscovery();
  // Allow system to stabilize and prevent watchdog resets
  esp_task_wdt_reset();

  static unsigned long lastNetworkCheck = 0;
  static String lastSSID = WiFi.SSID();
  String currentSSID = WiFi.SSID();

  // Only check network periodically to reduce overhead
  if (millis() - lastNetworkCheck > 10000)
  { // Every 10 seconds
    lastNetworkCheck = millis();

    // Monitor for network changes less frequently
    if (lastSSID != currentSSID)
    {
      Serial.print("Network change detected: ");
      Serial.print(lastSSID);
      Serial.print(" -> ");
      Serial.println(currentSSID);
      lastSSID = currentSSID;
    }

    // If WiFi is disconnected, try to reconnect without heavy operations
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi disconnected, attempting to reconnect");

      // Different approach based on network type
      if (currentNetworkType == NETWORK_CAMPUS)
      {
        WiFi.begin(SECRET_UCB_SSID);
      }
      else if (currentNetworkType == NETWORK_STUDIO)
      {
        WiFi.begin(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
      }
      else
      {
        WiFi.begin(SECRET_HOME_SSID, SECRET_WIFI_PASS);
      }

      // Check if we need to reinitialize UDP
      if (WiFi.status() == WL_CONNECTED && !UDPConnected)
      {
        connectLocalPort();
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED && UDPConnected)
  {
    // Even if not connected to cloud, read sensor data
    // Add new reading to the total
    total = total - readings[readIndex];              // Subtract the last reading
    readings[readIndex] = analogRead(sweatSensorPin); // Get a new reading
    total = total + readings[readIndex];              // Add the new reading to total
    readIndex = (readIndex + 1) % sampleSize;         // Advance to the next index

    // Calculate the average of the readings
    average = total / sampleSize;

    // Map the average and update min and max for OSC messages
    // Disabling for testing (04/02) - might be causing sensitivity issues with data fluctuations
    // average = map(average, 0, 2048, 0, 1023);

    if (average < 512 && !calibrated)
    {
      delay(5000); // Wait for a second to stabilize the sensor
      // skip the first reading if not calibrated
      Serial.println("Skipping first reading for calibration...");
      return;
    }

    if (!calibrated)
    {
      // Calibrate the sensor
      gSRUserMin = average;
      gSRUserMax = average;
      calibrated = true;
      String address = "/User_" + String(USER) + "/gsr/min";
      sendOSCMessage(address.c_str(), gSRUserMin);
      String address2 = "/User_" + String(USER) + "/gsr/max";
      sendOSCMessage(address2.c_str(), gSRUserMax);
    }
    else if (average < gSRUserMin)
    {
      // Update max and min values dynamically
      gSRUserMin = average;
      String address = "/User_" + String(USER) + "/gsr/min";
      sendOSCMessage(address.c_str(), gSRUserMin);
    }
    if (average > gSRUserMax)
    {
      gSRUserMax = average;
      String address = "/User_" + String(USER) + "/gsr/max";
      sendOSCMessage(address.c_str(), gSRUserMax);
    }
    // Send the current GSR value to Max/MSP
    gSRCurrentValue = average;

    // Send OSC messages only if connected
    String address = "/User_" + String(USER) + "/gsr/value";
    sendOSCMessage(address.c_str(), gSRCurrentValue);

    // Debug output (limited to avoid serial buffer issues)
    if (DEBUG && (millis() % 2000) < 100)
    { // Only print every ~2 seconds
      Serial.print("GSR value: ");
      Serial.println(gSRCurrentValue);
    }
  }

  // Don't run too fast to avoid network issues
  // (we will use this to control rate of OSC messages sent to Max/MSP)
  delay(600);
}

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// Connect to local UDP port for OSC communication
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  Udp.begin(inPort); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Send to audio computer
  if (audioIPDiscovered)
  {
    Udp.beginPacket(discoveredAudioIP, outPortAudio);
  }
  else
  {
    Udp.beginPacket(outIpAudio, outPortAudio);
  }
  msg.send(Udp);
  Udp.endPacket();

  // Send to visual computer (use discovered IP if available)
  if (visualIPDiscovered)
  {
    Udp.beginPacket(discoveredVisualIP, outPortVisual);
  }
  else
  {
    Udp.beginPacket(outIpVisual, outPortVisual);
  }
  msg.send(Udp);
  Udp.endPacket();

  // Free space
  msg.empty();

  // Debug message but skip if address string contains "/pulse"
  if (DEBUG && !strstr(address, "/pulse"))
  {
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
    Serial.print("...to both IP addresses: ");
    if (audioIPDiscovered)
    {
      Serial.print(discoveredAudioIP);
    }
    else
    {
      Serial.print(outIpAudio);
    }
    Serial.print("(audio) and ");
    if (visualIPDiscovered)
    {
      Serial.print(discoveredVisualIP);
    }
    else
    {
      Serial.print(outIpVisual);
    }
    Serial.println("(visual)");
  }
}