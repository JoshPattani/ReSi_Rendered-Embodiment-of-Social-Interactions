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

// HRV metrics
float rmssd; // Root mean square of successive differences
float sdann; // Standard deviation of the average NN intervals
float sdnn;  // Standard deviation of NN intervals

float hRCurrentValue; // Current heart rate
float hRUserMax;      // User-defined maximum heart rate
float hRUserMin;      // User-defined minimum heart rate

// Declarations
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void doThisOnConnect();
void doThisOnDisconnect();

void initProperties()
{

    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(rmssd, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(sdann, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(sdnn, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(hRCurrentValue, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(hRUserMax, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(hRUserMin, Permission::Read).publishOnChange(0.0);
}

// Create a connection handler - will be properly configured in main.cpp
SmartConnectionHandler AtlasPreferredConnection(SSID, PASS);