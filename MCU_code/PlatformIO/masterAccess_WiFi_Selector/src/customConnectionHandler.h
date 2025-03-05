/*
 * Custom connection handler for the GSR sensor project
 * This class extends the WiFiConnectionHandler class to provide custom behavior
 * for connecting to WiFi networks. It allows for preserving an existing connection
 * and using custom credentials based on the network type.
 *
 */
#ifndef CUSTOM_CONNECTION_HANDLER_H
#define CUSTOM_CONNECTION_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_ConnectionHandler.h>
#include "networkTypes.h"

class SmartConnectionHandler : public WiFiConnectionHandler
{
public:
    // Constructor - keep the original WiFiConnectionHandler constructor
    SmartConnectionHandler(const char *ssid, const char *pass) : WiFiConnectionHandler(ssid, pass),
                                                                 _networkType(NETWORK_UNKNOWN)
    {
        _preserveConnection = false;
        _customSsid = ssid; // Store the initial SSID as fallback
        _customPass = pass; // Store the initial password as fallback
        _connected = false;
        _fallbackSsid = ssid; // Keep original credentials as fallback
        _fallbackPass = pass;
    }

    // Set whether to preserve an existing WiFi connection
    void setPreserveConnection(bool preserve)
    {
        _preserveConnection = preserve;
    }

    // Store network type
    void setNetworkType(NetworkType type)
    {
        _networkType = type;
    }

    // Override with custom credentials
    void setCustomCredentials(const char *ssid, const char *pass)
    {
        _customSsid = ssid;
        _customPass = pass;
    }

    // Override connect method to respect existing connection if needed
    virtual void connect()
    {
        if (_preserveConnection && WiFi.status() == WL_CONNECTED)
        {
            Serial.println("SmartConnectionHandler: Using existing WiFi connection to " + WiFi.SSID());
            _connected = true;
            return; // Do nothing to maintain the existing connection
        }
        else if (_customSsid != nullptr && _networkType != NETWORK_UNKNOWN)
        {
            // Connect using custom credentials
            Serial.print("SmartConnectionHandler: Connecting with network type credentials: ");
            Serial.println(_customSsid);
            WiFi.disconnect();
            WiFi.begin(_customSsid, _customPass);
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20)
            {
                delay(500);
                Serial.print(".");
                attempts++;
            }
            _connected = (WiFi.status() == WL_CONNECTED);
        }
        else
        {
            // Use parent class connection method
            WiFiConnectionHandler::connect();
            _connected = (WiFi.status() == WL_CONNECTED);
        }
    }

    // Override disconnect method to respect campus WiFi
    virtual void disconnect()
    {
        if (_preserveConnection && WiFi.status() == WL_CONNECTED)
        {
            Serial.println("SmartConnectionHandler: Preserving WiFi connection");
            // Don't disconnect
        }
        else
        {
            WiFiConnectionHandler::disconnect();
            _connected = false;
        }
    }

    // Override update method to prevent cloud from changing the connection
    virtual NetworkConnectionState update()
    {
        if (_preserveConnection && WiFi.status() == WL_CONNECTED)
        {
            // Force cloud to use the existing connection
            return NetworkConnectionState::CONNECTED;
        }
        // Since WiFiConnectionHandler::update() doesn't exist, implement our own logic
        if (WiFi.status() == WL_CONNECTED)
        {
            _connected = true;
            return NetworkConnectionState::CONNECTED;
        }
        else
        {
            _connected = false;
            return NetworkConnectionState::DISCONNECTED;
        }
    }

    // Get the current connection status
    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    // Override for better control over reconnection attempts
    void check()
    {
        if (_preserveConnection && WiFi.status() == WL_CONNECTED)
        {
            // Do nothing - we want to keep this connection
            _connected = true;
        }
        else
        {
            // If not preserving, use parent class behavior
            WiFiConnectionHandler::check();
        }
    }

private:
    bool _preserveConnection;
    bool _connected;
    NetworkType _networkType;
    const char *_customSsid;
    const char *_customPass;
    const char *_fallbackSsid;
    const char *_fallbackPass;
};

#endif // CUSTOM_CONNECTION_HANDLER_H