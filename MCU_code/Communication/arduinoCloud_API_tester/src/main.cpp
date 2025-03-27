/*
  This is a basic example sketch showing how to connect to the Arduino IoT Cloud and publish to a Thing using PlatformIO.

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  int value1;

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

  // For home/studio networks, this will handle the WiFi connection
  // For campus network, we're already connected, so it will just use the existing connection
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
}

void loop()
{
  // Dont run if not connected to Arduino IoT Cloud or UDP
  if (!ArduinoCloud.connected() && !UDPConnected)
  {
    ArduinoCloud.update();
    return;
  }
  ArduinoCloud.update();

  // You can add your code here
  // value1 = analogRead(A0);
  // sendOSCMessage("/value1", value1);
  //

  delay(500); // Adjust delay as needed
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

// Connect to local port for OSC communication

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
