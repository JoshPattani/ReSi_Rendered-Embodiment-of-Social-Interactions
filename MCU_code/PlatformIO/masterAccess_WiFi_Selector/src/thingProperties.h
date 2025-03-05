#include "arduino_secrets.h"
#include <Arduino.h>
#include <ArduinoIoTCloud.h>
#include <WiFi.h>
#include "customConnectionHandler.h"

const char DEVICE_LOGIN_NAME[] = SECRET_DEVICE_ID;

const char SSID[] = "";                      // Empty SSID to prevent automatic connections
const char PASS[] = "";                      // Empty password
const char DEVICE_KEY[] = SECRET_DEVICE_KEY; // Secret device password

// IP and port for OSC communication
const IPAddress outIp(SECRET_TARGET_IP); // Use the secret target IP defined in arduino_secrets.h
// Alternatively: const IPAddress outIp;  // Declare first, initialize in setup()
const unsigned int outPort = SECRET_TARGET_PORT; // Target port. Must match the port in your Max/MSP patch.

int value1 = 0;

// Declarations
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void doThisOnConnect();
void doThisOnDisconnect();

void initProperties()
{

    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(value1, Permission::Read).publishOnChange(0.0);
}

// Create a connection handler - will be properly configured in main.cpp
SmartConnectionHandler AtlasPreferredConnection(SSID, PASS);