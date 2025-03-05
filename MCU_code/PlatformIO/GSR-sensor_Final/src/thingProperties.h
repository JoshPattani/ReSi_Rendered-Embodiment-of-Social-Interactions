#include "arduino_secrets.h"
#include <Arduino.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>

// user identifier (for multi-participant synchronization)
const String USER_A = "A";  // Participant A
const String USER_B = "B";  // Participant B
const String USER = USER_A; // Current participant identifier

const char DEVICE_LOGIN_NAME[] = SECRET_DEVICE_ID;

const char SSID[] = SECRET_HOME_SSID;        // Network SSID (name)
const char PASS[] = SECRET_WIFI_PASS;        // Network password (use for WPA, or use as key for WEP)
const char DEVICE_KEY[] = SECRET_DEVICE_KEY; // Secret device password

// IP and port for OSC communication
const IPAddress outIp(SECRET_TARGET_IP); // Use the secret target IP defined in arduino_secrets.h
// Alternatively: const IPAddress outIp;  // Declare first, initialize in setup()
const unsigned int outPort = SECRET_TARGET_PORT; // Target port. Must match the port in your Max/MSP patch.

int gSRCurrentValue;
int gSRUserMin = 1023;
int gSRUserMax = 4095;

// Declarations
void sendOSCMessage(const char *address, float value);
void connectLocalPort();
void doThisOnConnect();
void doThisOnDisconnect();

void initProperties()
{

    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(gSRCurrentValue, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(gSRUserMax, Permission::Read).publishOnChange(0.0);
    ArduinoCloud.addProperty(gSRUserMin, Permission::Read).publishOnChange(0.0);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);