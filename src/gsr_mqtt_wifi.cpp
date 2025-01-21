/*
  Title: WiFi OSC Sensor Transmission with MQTT
  Description: Arduino sketch to read from an analog sensor pin and send data via MQTT and UDP over WiFi.
  Author: [Your Name]
  Date: [Date]

  Dependencies:
    - WiFiNINA or corresponding WiFi library for your board.
    - WiFiUDP for UDP communication.
    - OSCMessage library (CNMAT-OSC) for building and sending OSC packets.
    - ArduinoMqttClient library for MQTT communication.

  Configuration:
    1. Update WiFi credentials: WIFI_SSID, WIFI_PASSWORD
    2. Update sensor pin: SENSOR_PIN
    3. Update target IP: TARGET_IP
    4. Update target port: TARGET_PORT
    5. Set the sampling interval: INTERVAL_MS
*/
#include <Arduino.h>           // Required for platformio
#include <Wire.h>              // I2C communication
#include <WiFi.h>              // ESP32 WiFi
#include <WiFiUDP.h>           // Required for UDP over WiFi
#include <OSCMessage.h>        // CNMAT-OSC library for building OSC messages
#include <ArduinoMqttClient.h> // Arduino MQTT client

/*********** User Configuration ***********/
// WiFi credentials
#include "arduino_secrets.h"
// WiFi credentials
const char *SSID = SECRET_SSID;
const char *PASS = SECRET_PASS;

// WiFi and MQTT clients
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// MQTT Broker credentials
const char *mqtt_server = "test.mosquitto.org"; // Replace with your broker's URL (e.g., "broker.hivemq.com")
int port = 1883;
const char *mqtt_topic = "gsr-sensor-phalange"; // Topic to publish data to

// Network target
IPAddress TARGET_IP(192, 168, 0, 8);    // e.g., 192.168.1.100
const unsigned int TARGET_PORT = 42069; // e.g., 8000

// Sensor pin and sampling interval
const int SENSOR_PIN = A0;
const unsigned long INTERVAL_MS = 100; // e.g., 100 ms sampling interval
/*****************************************/

WiFiUDP Udp;
unsigned long previousMillis = 0;

void connectToWiFi()
{
    Serial.print("Connecting to WiFi...");
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        while (true)
            ;
    }
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n🛜 WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    // Initialize UDP
    Udp.begin(WiFi.localIP()); // Bind local UDP port (can be any valid port)
    Serial.println("UDP initialized.");
}

void connectToMQTT()
{
    Serial.println("Attempting to connect to MQTT broker: ");
    Serial.println(mqtt_server);
    while (!mqttClient.connect(mqtt_server, port))
    {
        Serial.print("❌ MQTT connection failed! Error: ");
        Serial.println(mqttClient.connectError());
        Serial.println(" Retrying in 5 seconds...");
        delay(5000);
    }
    if (mqttClient.connected())
    {
        Serial.println("✨Connection successful!✨");
        Serial.println();
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

    // Connect to WiFi
    connectToWiFi();

    // Connect to MQTT
    connectToMQTT();
}

void loop()
{
    // Ensure MQTT connection
    if (!mqttClient.connected())
    {
        connectToMQTT();
    }

    // Check timing for sampling interval
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= INTERVAL_MS)
    {
        previousMillis = currentMillis;

        // Read sensor value
        int sensorValue = analogRead(SENSOR_PIN);

        // Build OSC message
        OSCMessage oscMsg("/sensor");
        oscMsg.add((float)sensorValue); // Convert int to float if desired

        // Send message via UDP to specified IP and port
        Udp.beginPacket(TARGET_IP, TARGET_PORT);
        oscMsg.send(Udp);
        Udp.endPacket();
        oscMsg.empty(); // Clear message buffer

        // Build MQTT message
        mqttClient.beginMessage(mqtt_topic);
        mqttClient.print(sensorValue);
        mqttClient.endMessage();

        // Debug (optional)
        Serial.print("Sent OSC /sensor ");
        Serial.println(sensorValue);
    }
}
