/*
  Title: WiFi OSC Sensor Transmission
  Description: Arduino sketch to read from an analog sensor pin and wirelessly send data via OSC over WiFi.
  Author: [Your Name]
  Date: [Date]

  Dependencies:
    - WiFi library for your board.
    - WiFiUDP for UDP communication.
    - OSCMessage library (CNMAT-OSC) for building and sending OSC packets.

*/
#include <Arduino.h>    // Required for platformio
#include <WiFi.h>       // ESP32 WiFi
#include <WiFiUDP.h>    // Required for UDP over WiFi
#include <OSCMessage.h> // CNMAT-OSC library for building OSC messages

/*********** User Configuration ***********/
// WiFi credentials
#include "arduino_secrets.h"
// WiFi credentials
const char *SSID = SECRET_SSID;
const char *PASS = SECRET_PASS;

// WiFi client
WiFiClient wifiClient;
WiFiUDP Udp; // UDP over WiFi

// Target IP Address & UDP port
IPAddress targetIP(192, 168, 0, 8); // IP address of the computer running the OSC receiver
const uint16_t TARGET_PORT = 42069;

// Sensor pin and sampling interval
const int GSR_SENSOR_PIN = A0;
int gsr_value = 0;
int gsr_average = 0;
const unsigned long INTERVAL_MS = 100; // e.g., 100 ms sampling interval
unsigned long previousMillis = 0;
/*****************************************/

void connectToWiFi()
{
  Serial.print("Connecting to WiFi...");
  WiFi.begin(SSID, PASS);

  // Enable LED to indicate WiFi connection status
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink LED to indicate status
    digitalWrite(LED_RED, HIGH);
    delay(1000);
    digitalWrite(LED_RED, LOW);
    Serial.print(".");
  }

  // Turn on LED to indicate successful connection
  digitalWrite(LED_GREEN, HIGH);

  // Serial print WiFi connection details
  Serial.println("\n🛜 WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

int read_gsr_sensor(int SENSOR_PIN = GSR_SENSOR_PIN)
{
  long sum = 0;
  for (int i = 0; i < 10; i++) // Average the 10 measurements to remove the glitch
  {
    gsr_value = analogRead(SENSOR_PIN);
    sum += gsr_value;
    delay(5);
  }
  gsr_average = sum / 10;
  // Serial.print("GSR sensor value: ");
  // Serial.println(gsr_average);

  return gsr_average;
}

void setup()
{
  Serial.begin(115200);

  // Connect to WiFi
  connectToWiFi();

  // Initialize UDP
  Udp.begin(42069); // Bind local UDP port (here we use the memeport)
  Serial.println("UDP initialized.");

  // Initialize LED for comms indication
  pinMode(LED_BLUE, OUTPUT);
}

void loop()
{
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    // Blocking loop to maintain WiFi connection
    while (WiFi.status() != WL_CONNECTED)
    {
      // Retry WiFi connection
      Serial.println("WiFi connection lost. Reconnecting...");
      connectToWiFi();
    }
  }

  // Check timing for sampling interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL_MS)
  {
    previousMillis = currentMillis;

    // Read sensor value and send it via OSC
    OSCMessage msg("/analog/0");
    msg.add((int32_t)read_gsr_sensor(GSR_SENSOR_PIN)); // add the sensor value to the OSC message

    Udp.beginPacket(targetIP, TARGET_PORT);
    msg.send(Udp);   // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty();     // free space occupied by message

    // Blink LED to indicate data transmission
    digitalWrite(LED_BLUE, HIGH);
    delay(10);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, HIGH);
    delay(5);
    digitalWrite(LED_RED, LOW);

    // Delay to avoid flooding the network
    delay(10);

    // Debug (optional)
    // Serial.print("Sent GSR sensor value: ");
    // Serial.println(gsr_value);
    // Serial.print("To IP: ");
    // Serial.println(targetIP);
    // Serial.print("To port: ");
    // Serial.println(TARGET_PORT);
    // Serial.println();
  }
}
