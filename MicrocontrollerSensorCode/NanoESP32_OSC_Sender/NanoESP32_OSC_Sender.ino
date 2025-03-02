/*
 * OSC Sender for Arduino Nano ESP32
 *
 * This sketch connects to WiFi, reads sensor values, formats them as OSC messages,
 * and sends them via UDP to a specified IP address and port.
 * Optionally still updates Arduino Cloud for visualization.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h> // You'll need to install this library via Library Manager
#include "ArduinoIoTCloud.h"
#include "arduino_secrets.h"

// Network credentials
const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

// OSC configuration
const IPAddress oscTargetIP(192, 168, 0, 8); // Target IP address (where your Max/MSP is running)
const unsigned int oscPort = 12345;          // Target port

// Arduino Cloud config - can be used for visualization
const char THING_ID[] = SECRET_THING_ID;
const char DEVICE_ID[] = SECRET_DEVICE_ID;

// Sample analog pin for demonstration
const int sensorPin1 = A0; // Analog input pin
const int sensorPin2 = A1; // Second analog input pin

// Update intervals
const unsigned long oscSendInterval = 50;     // 50ms = 20Hz send rate
const unsigned long cloudSendInterval = 1000; // 1000ms = 1Hz cloud update rate

// Timers
unsigned long lastOscSendTime = 0;
unsigned long lastCloudSendTime = 0;

// Create UDP instance
WiFiUDP udp;

// Cloud variables
CloudFloat sensorValue1;
CloudFloat sensorValue2;

void setupCloudVariables()
{
  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.setDeviceId(DEVICE_ID);

  ArduinoCloud.addProperty(sensorValue1, "sensor1", READ, ON_CHANGE);
  ArduinoCloud.addProperty(sensorValue2, "sensor2", READ, ON_CHANGE);
}

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

  // Setup cloud connection (optional for visualization)
  setupCloudVariables();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Setup analog pins
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
}

void loop()
{
  // Handle cloud connection
  ArduinoCloud.update();

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

  // Update cloud at specified interval (less frequent to avoid rate limits)
  if (currentTime - lastCloudSendTime >= cloudSendInterval)
  {
    lastCloudSendTime = currentTime;
    sensorValue1 = value1;
    sensorValue2 = value2;
  }
}

void sendOSCMessage(const char *address, float value)
{
  // Create an OSC message
  OSCMessage msg(address);
  msg.add(value);

  // Begin UDP packet
  udp.beginPacket(oscTargetIP, oscPort);

  // Write OSC message to UDP
  msg.send(udp);

  // End packet and send
  udp.endPacket();

  // Free space
  msg.empty();

  Serial.print("Sent OSC message: ");
  Serial.print(address);
  Serial.print(" ");
  Serial.println(value);
}
