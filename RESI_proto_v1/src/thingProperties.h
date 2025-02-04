#include <ArduinoIoTCloud.h>
#include <WiFi.h> // Add board-specific WiFi header (e.g., for ESP32)
#include <Arduino_ConnectionHandler.h>
#include <ESP32WiFiConnectionHandler.h> // Use the concrete connection handler for ESP32
#include "arduino_secrets.h"

const char DEVICE_LOGIN_NAME[] = "67cd42e1-e386-4a17-a8b7-e4daf07c5d72";

const char *SSID = SECRET_SSID;             // Network SSID
const char *PASS = SECRET_PASS;             // Network password
const char *DEVICE_KEY = SECRET_DEVICE_KEY; // Secret device password

int gSR;

void initProperties()
{
    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(gSR, READ, ON_CHANGE, NULL);
}

ESP32WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
