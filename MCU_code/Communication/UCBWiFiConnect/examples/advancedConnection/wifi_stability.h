#ifndef WIFI_STABILITY_H
#define WIFI_STABILITY_H

// #include <Arduino.h> // Include Arduino header if using PlatformIO in VSCode or similar IDEs
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

// WiFi stability manager to prevent crashes and resets
class WiFiStabilityManager
{
public:
    static void configureDevice()
    {
        // Disable WiFi power saving - prevents connection drops
        esp_wifi_set_ps(WIFI_PS_NONE);
        WiFi.setSleep(false);

        // Ensure watchdog timeout is extended
        esp_task_wdt_init(30, false);

        // Set WiFi to higher tx power
        esp_wifi_set_max_tx_power(80); // Maximum power
    }

    static void waitForStableConnection(int timeoutMs = 10000)
    {
        unsigned long startTime = millis();
        bool isStable = false;

        Serial.println("Waiting for stable WiFi connection...");

        // Make several connection checks over time
        while (!isStable && (millis() - startTime < timeoutMs))
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                // Connection seems stable, wait a bit more
                delay(500);
                if (WiFi.status() == WL_CONNECTED)
                {
                    isStable = true;
                }
            }
            delay(100);
            Serial.print(".");
        }

        Serial.println("");
        if (isStable)
        {
            Serial.println("WiFi connection is stable");
            // Print connection details
            Serial.print("Connected to: ");
            Serial.println(WiFi.SSID());
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            Serial.print("Signal strength (RSSI): ");
            Serial.println(WiFi.RSSI());
        }
        else
        {
            Serial.println("WiFi connection is not stable");
        }
    }

    // Connect to WiFi with proper stability measures
    static bool connectToNetwork(const char *ssid, const char *password)
    {
        Serial.print("Connecting to network: ");
        Serial.println(ssid);

        // First disconnect any existing connection
        WiFi.disconnect(true);
        delay(1000);

        // Configure WiFi for stability
        configureDevice();

        // Begin connection
        WiFi.begin(ssid, password);

        // Wait for connection with timeout
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30)
        {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println("");

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("Successfully connected to WiFi");
            waitForStableConnection();
            return true;
        }
        else
        {
            Serial.println("Failed to connect to WiFi");
            return false;
        }
    }
};

#endif // WIFI_STABILITY_H