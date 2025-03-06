/*
 * WiFi Network Auto-Selection with OSC Messaging for GSR Sensor
 *
 * This version removes Arduino IoT Cloud connectivity while preserving
 * the ability to automatically detect and connect to different network
 * environments and send OSC messages.
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

// IP and port for OSC communication
const IPAddress outIp(SECRET_TARGET_IP); // Use the secret target IP defined in arduino_secrets.h
// Alternatively: const IPAddress outIp;  // Declare first, initialize in setup()
const unsigned int outPort = SECRET_TARGET_PORT; // Target port. Must match the port in your Max/MSP patch.

// Debug flag
const bool DEBUG = true;

// Function prototypes
void connectLocalPort();
void sendOSCMessage(const char *address, float value);

// GSR sensor variables
int gSRCurrentValue = 0;
int gSRUserMax = 0;
int gSRUserMin = 1023; // Default to maximum value

const int sweatSensorPin = A0;
const int sampleSize = 5; // Reduce the number of samples for less smoothing (try 2 or 3 as well)
bool calibrated = false;

int readings[sampleSize]; // Array to hold sensor readings for moving average
int readIndex = 0;        // Current index for the moving average
int total = 0;            // Running total for the moving average
int average = 0;          // Average value

void setup()
{
  // Initialize serial and wait for port to open
  Serial.begin(9600);
  delay(4000); // Give more time for serial to initialize

  Serial.println("Starting RESI WiFi detection and connection (No Cloud Version)...");

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

  // Even if not connected to cloud, read sensor data
  // Add new reading to the total
  total = total - readings[readIndex];              // Subtract the last reading
  readings[readIndex] = analogRead(sweatSensorPin); // Get a new reading
  total = total + readings[readIndex];              // Add the new reading to total
  readIndex = (readIndex + 1) % sampleSize;         // Advance to the next index

  // Calculate the average of the readings
  average = total / sampleSize;

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

  // Update variables and send OSC messages
  gSRCurrentValue = map(average, 0, 1023, 0, 500);

  if (WiFi.status() == WL_CONNECTED && UDPConnected)
  {
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
  delay(200);
}

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

// Connect to local UDP port for OSC communication
void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  Udp.begin(outPort); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Send an OSC message
void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Begin UDP packet
  Udp.beginPacket(outIp, outPort);

  // Write OSC message to UDP
  msg.send(Udp);

  // End packet and send
  Udp.endPacket();

  // Free space
  msg.empty();

  if (DEBUG)
  {
    Serial.print("Sent OSC message: ");
    Serial.print(address);
    Serial.print(" ");
    Serial.println(value);
    Serial.print("...to IP address: ");
    Serial.print(outIp);
    Serial.print(" on port: ");
    Serial.println(outPort);
  }
}