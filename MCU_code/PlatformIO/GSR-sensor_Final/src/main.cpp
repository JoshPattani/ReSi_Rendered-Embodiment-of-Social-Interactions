/*
  Sketch associated with the Arduino IoT Cloud Thing "GSR-sensor_OSC"
  https://create.arduino.cc/cloud/things/c5d93ee8-f2ff-4fff-8f3f-86e363dff34e

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  int gSRCurrentValue;
  int gSRUserMax;
  int gSRUserMin;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/
#include <Arduino.h>
#include "networkTypes.h"            // Include first
#include "customConnectionHandler.h" // Then custom connection handler
#include "thingProperties.h"         // Then thing properties
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

const int sweatSensorPin = A0;
const int sampleSize = 5; // Reduce the number of samples for less smoothing (try 2 or 3 as well)

int readings[sampleSize]; // Array to hold sensor readings for moving average
int readIndex = 0;        // Current index for the moving average
int total = 0;            // Running total for the moving average
int average = 0;          // Average value

// Debug flag
const bool DEBUG = true;

void setup()
{
  // Initialize serial and wait for port to open
  Serial.begin(9600);
  delay(2000); // Give more time for serial to initialize

  Serial.println("Starting RESI WiFi detection and connection...");

  // Configure device for stability
  WiFiStabilityManager::configureDevice();

  // First detect network type (but don't connect yet)
  currentNetworkType = detectNetworkType();

  // Connect to the appropriate network based on type
  bool wifiConnected = false;

  if (currentNetworkType == NETWORK_HOME)
  {
    Serial.println("Home network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_HOME_SSID, SECRET_WIFI_PASS);
    AtlasPreferredConnection.setCustomCredentials(SECRET_HOME_SSID, SECRET_WIFI_PASS);
    AtlasPreferredConnection.setNetworkType(NETWORK_HOME);
  }
  else if (currentNetworkType == NETWORK_STUDIO)
  {
    Serial.println("Studio network detected");
    wifiConnected = WiFiStabilityManager::connectToNetwork(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
    AtlasPreferredConnection.setCustomCredentials(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
    AtlasPreferredConnection.setNetworkType(NETWORK_STUDIO);
  }
  else if (currentNetworkType == NETWORK_CAMPUS)
  {
    Serial.println("Campus network detected");
    // Try campus connection (this will handle authentication too)
    if (handleUCBWirelessAuth())
    {
      Serial.println("Successfully connected to campus network");
      AtlasPreferredConnection.setPreserveConnection(true);
      AtlasPreferredConnection.setNetworkType(NETWORK_CAMPUS);
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
    AtlasPreferredConnection.setCustomCredentials(SECRET_HOME_SSID, SECRET_WIFI_PASS);
  }

  // Only initialize Arduino Cloud if WiFi is connected
  if (WiFi.status() == WL_CONNECTED)
  {
    // Initialize properties first
    initProperties();

    Serial.println("Initializing Arduino IoT Cloud connection...");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    Serial.print("Connected to: ");
    Serial.println(WiFi.SSID());

    // Give system time to stabilize before cloud initialization
    delay(3000);

    try
    {
      // Initialize cloud with try-catch to prevent crashes
      ArduinoCloud.begin(AtlasPreferredConnection);
      ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
      ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

      Serial.println("Arduino Cloud initialized");
      setDebugMessageLevel(2);
      ArduinoCloud.printDebugInfo();

      // First update cycle
      ArduinoCloud.update();
    }
    catch (const std::exception &e)
    {
      Serial.print("Exception during cloud initialization: ");
      Serial.println(e.what());
    }
    catch (...)
    {
      Serial.println("Unknown exception during cloud initialization");
    }
  }
  else
  {
    Serial.println("WiFi not connected - skipping Arduino IoT Cloud initialization");
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

  // Try to initialize UDP anyway for local communications
  if (!UDPConnected)
  {
    connectLocalPort();
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
    }
  }

  // Handle ArduinoCloud updates safely
  if (WiFi.status() == WL_CONNECTED)
  {
    try
    {
      ArduinoCloud.update();
    }
    catch (...)
    {
      Serial.println("Error in ArduinoCloud.update()");
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
  }
}

void connectLocalPort()
{
  delay(1000);
  Serial.println("Connecting to UDP port...");
  Udp.begin(outPort); // Local port to listen on (doesn't matter much for sending)
  UDPConnected = true;
  Serial.println("UDP connection established!");
}

// Callback function to run when connected to Arduino IoT Cloud
void doThisOnConnect()
{
  Serial.println("Connected to Arduino IoT Cloud!");

  // Verify we're still on the correct network for campus connections
  if (currentNetworkType == NETWORK_CAMPUS && WiFi.SSID() != "UCB Wireless")
  {
    Serial.print("WARNING: Network changed from UCB Wireless to ");
    Serial.println(WiFi.SSID());
    Serial.println("Attempting to reconnect to campus network...");

    // Try to reconnect to campus network
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(SECRET_UCB_SSID);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == "UCB Wireless")
    {
      Serial.println("Successfully reconnected to campus network!");
    }
    else
    {
      Serial.println("Failed to reconnect. Using current network: " + WiFi.SSID());
    }
  }

  if (!UDPConnected)
  {
    connectLocalPort();
    Serial.println("Ready to send OSC messages");
  }
}

// Callback function to run when disconnected from Arduino IoT Cloud
void doThisOnDisconnect()
{
  Serial.println("Disconnected from Arduino IoT Cloud!");
  UDPConnected = false;

  WiFi.disconnect();
  delay(1000);
  WiFi.begin(SECRET_UCB_SSID);
  Serial.println("Attempting to reconnect to the campus network...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println("Reconnected successfully!");

  if (WiFi.status() == WL_CONNECTED)
  {
    connectLocalPort();
    Serial.println("Ready to send OSC messages");
  }
}
