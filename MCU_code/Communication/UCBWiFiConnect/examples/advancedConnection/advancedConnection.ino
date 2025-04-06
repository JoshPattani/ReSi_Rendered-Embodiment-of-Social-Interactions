/**
 * Campus WiFi Connection with Captive Portal Support
 *
 * This sketch enables ESP32 devices to connect to university WiFi networks
 * that use captive portal authentication. It's specifically designed for
 * UCB Wireless but can work with other similar networks.
 *
 * Features:
 * - Automatic captive portal detection
 * - Authentication to the captive portal
 * - Support for form-based and redirect-based portals
 * - Support for authentication tokens
 *
 * Setup:
 * 1. Create wifi_credentials.h with your credentials
 * 2. Adjust configuration parameters if needed
 * 3. Upload to your ESP32
 *
 * Created for RESI (Rendered Embodiment of Social Interactions) 2025
 * Author: Josh P
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
#include "wifi_credentials.h"

// Network configuration
WiFiUDP Udp;                           // Create a UDP instance for OSC communication
bool UDPConnected = false;             // Flag to check if UDP connection is established
extern NetworkType currentNetworkType; // Reference the type from wifiHandler.h

// IP and port for OSC communication
const IPAddress outIp(SECRET_TARGET_IP); // Use the secret target IP defined in arduino_secrets.h
// Alternatively: const IPAddress outIp;  // Declare first, initialize in setup()
const unsigned int outPort = SECRET_TARGET_PORT; // Target port. Must match the port in your Max/MSP patch.

// Example sensor variables (replace with your actual sensor code)
int sensorValue = 0;

// Debug flag
const bool DEBUG = true;

// Function prototypes
void connectLocalPort();
void sendOSCMessage(const char *address, float value);

void setup()
{
  // Initialize serial and wait for port to open
  Serial.begin(9600);
  delay(2000); // Give more time for serial to initialize

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

  // Read sensor data (replace this with your actual sensor reading code)
  sensorValue = analogRead(A0); // Example sensor reading

  // Send OSC messages if connected
  if (WiFi.status() == WL_CONNECTED && UDPConnected)
  {
    // Send OSC messages only if connected
    sendOSCMessage("/sensor/value", sensorValue);

    // Debug output (limited to avoid serial buffer issues)
    if (DEBUG && (millis() % 2000) < 100)
    { // Only print every ~2 seconds
      Serial.print("Sensor value: ");
      Serial.println(sensorValue);
    }
  }

  // Don't run too fast to avoid network issues
  delay(100);
}

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
  }
}