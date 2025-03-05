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
#include "thingProperties.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "wifiHandler.h"

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
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  Serial.println("Starting ReSi WiFi detection and connection...");

  // First detect network and potentially connect if it's the campus network
  try
  {
    connectToWiFi();
  }
  catch (const std::exception &e)
  {
    Serial.println("Error in WiFi connection process");
    Serial.println(e.what());
  }

  // Defined in thingProperties.h
  initProperties();

  // This will use the WiFiConnectionHandler initialized in thingProperties.h
  // For campus network, we're already connected, so it will just use the existing connection
  if (currentNetworkType == NETWORK_STUDIO)
  {
    // If we're on the studio network, we'll need to reconnect with the correct credentials
    WiFi.disconnect();
    WiFi.begin(SECRET_STUDIO_SSID, SECRET_STUDIO_WIFI_PASS);
    Serial.println("Connecting to studio WiFi network...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nConnected to studio network!");
    }
    else
    {
      Serial.println("\nFailed to connect to studio network");
    }
  }

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("Initializing GSR sensor with OSC output...");

  // initialize gSRUserMin and gSRUserMax
  gSRUserMin = 1023;
  gSRUserMax = 0;

  // Initialize readings array
  for (int i = 0; i < sampleSize; i++)
  {
    readings[i] = 0;
  }
}

void loop()
{
  // Dont run if not connected to Arduino IoT Cloud
  if (!ArduinoCloud.connected() && !UDPConnected)
  {
    ArduinoCloud.update();
    return;
  }
  ArduinoCloud.update();

  // Add new reading to the total
  total = total - readings[readIndex];              // Subtract the last reading
  readings[readIndex] = analogRead(sweatSensorPin); // Get a new reading
  total = total + readings[readIndex];              // Add the new reading to total
  readIndex = (readIndex + 1) % sampleSize;         // Advance to the next index

  // Calculate the average of the readings
  average = total / sampleSize;

  // Update max and min values dynamically
  if (average < gSRUserMin)
  {
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

  // You can try adding an offset or scaling the values differently here:
  // Example: map the value to a broader range
  gSRCurrentValue = map(average, 0, 1023, 0, 500); // Broadening the range

  // Send the GSR value through OSC with User ID tag, i,e. "/User_A/gsr/value"
  String address = "/User_" + String(USER) + "/gsr/value";
  sendOSCMessage(address.c_str(), gSRCurrentValue);

  // Create OSC message
  // OSCMessage msg("/" + String(USER) + "/gsr/value");
  // msg.add(gSRCurrentValue);

  // // Send the OSC message
  // Udp.beginPacket(outIp, outPort);
  // msg.send(Udp);
  // Udp.endPacket();

  // // Clear the message
  // msg.empty();
  if (DEBUG)
  {
    // Also print to Serial for debugging
    Serial.print("Sent GSR value: ");
    Serial.println(gSRCurrentValue);
    // Serial.print(" (Min: ");
    // Serial.print(gSRUserMin);
    // Serial.print(", Max: ");
    // Serial.print(gSRUserMax);
    // Serial.println(")");
  }

  delay(500); // Adjust delay to make fluctuations more visible
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
  if (!UDPConnected)
  {
    connectLocalPort();
    Serial.println("Ready to send OSC messages");
  }
}

void doThisOnDisconnect()
{
  Serial.println("Disconnected from Arduino IoT Cloud.");
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  delay(1000);
  UDPConnected = false;
}