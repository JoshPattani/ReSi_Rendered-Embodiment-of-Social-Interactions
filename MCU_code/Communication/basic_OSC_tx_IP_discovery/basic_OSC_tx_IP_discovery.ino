/*
 * Basic OSC Sender for Arduino Nano ESP32
 *
 * This sketch connects to WiFi, reads sensor values, formats them as OSC messages,
 * and sends them via UDP to a specified IP address and port.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h> // You'll need to install this library via Library Manager
#include "arduino_secrets.h"

// Network credentials
const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

// OSC configuration
const IPAddress oscTargetIP(127, 0, 0, 1); // Target IP address (where your Max/MSP is running)
const unsigned int oscPort = 12345;        // Target port

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
// --------------------------------------------- //

// =============================== //
// ***** Function Prototypes ***** //
// =============================== //

// Function prototypes
void listenForDiscovery();
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void sendAcknowledgment(IPAddress targetIP, unsigned int targetPort, const char *type, const char *user);

// Sample analog pin for demonstration
const int sensorPin1 = A0; // Analog input pin
const int sensorPin2 = A1; // Second analog input pin

// =============================== //
// ****** OSC Communication ****** //
// =============================== //

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

// OSC message addressing
// "/arduino/acknowledgment" -> Acknowledgment message
// "/debug" -> Debug message

// Update intervals
const unsigned long oscSendInterval = 50; // 50ms = 20Hz send rate

// Timers
unsigned long lastOscSendTime = 0;

// Create UDP instance
WiFiUDP udp;

// Connect to local UDP port for OSC communication
void connectLocalPort()
{
  delay(1000);
  if (DEBUG)
    Serial.println("Connecting to UDP port...");
  Udp.begin(inPort); // Local port to listen on (only matters for IP discovery, not much for sending)
  UDPConnected = true;
  if (DEBUG)
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
// --------------------------------------------- //

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting OSC Sender...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Begin UDP communication
  udp.begin(oscPort);
  Serial.print("UDP started on port: ");
  Serial.println(oscPort);

  // Setup analog pins
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
}

void loop()
{
  // Listen for discovery messages from Max
  listenForDiscovery();

  // Read sensor values
  int rawValue1 = analogRead(sensorPin1);
  int rawValue2 = analogRead(sensorPin2);

  // Map analog reading (0-4095 on ESP32) to 0.0-1.0 float
  float value1 = map(rawValue1, 0, 4095, 0, 1000) / 1000.0;
  float value2 = map(rawValue2, 0, 4095, 0, 1000) / 1000.0;

  // Current time
  unsigned long currentTime = millis();

  // Send OSC messages at specified interval
  if (currentTime - lastOscSendTime >= oscSendInterval)
  {
    lastOscSendTime = currentTime;
    sendOSCMessage("/sensor1", value1);
    sendOSCMessage("/sensor2", value2);
  }

  // Add any other processing or logic here
}