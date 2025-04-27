/*
    WiFi Handler Library for ESP32
    This library provides functions to handle WiFi connections
    and captive portal authentication for ESP32 devices.
    It includes a smart connection function that can detect
    known networks and handle captive portals automatically.

    "Code is a great equalizer", Josh P
*/

// #include <Arduino.h> // Include Arduino header if using PlatformIO in VSCode or similar IDEs
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "esp_wifi.h" // Include ESP32 WiFi header for esp_wifi_set_ps
#include "arduino_secrets.h"
#include "networkTypes.h" // Include the network types definition

// Add these external declarations to access needed variables
extern const IPAddress outIp;
extern const unsigned int outPort;

// WiFi credentials
const char *ssid = SECRET_UCB_SSID; // Your WiFi SSID

// Your university login credentials
const char *identikey = SECRET_IDENTIKEY; // Your university identikey
const char *password = SECRET_PASS;       // Your university password

// Add this global variable to track the selected network
NetworkType currentNetworkType = NETWORK_UNKNOWN;

// Variables to store portal information
String captivePortalURL = "";
String authURL = "";
String authToken = "";

// URL encoding function
String urlEncode(String str)
{
    String encodedString = "";
    char c;
    char code0;
    char code1;

    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);

        if (c == ' ')
        {
            encodedString += '+';
        }
        else if (isAlphaNumeric(c))
        {
            encodedString += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
                code1 = (c & 0xf) - 10 + 'A';
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
                code0 = c - 10 + 'A';
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }

    return encodedString;
}

// Function to scan and identify available networks
NetworkType detectNetworkType()
{
    Serial.println("Scanning for known networks...");
    int numNetworks = WiFi.scanNetworks();

    for (int i = 0; i < numNetworks; i++)
    {
        String ssid = WiFi.SSID(i);
        Serial.print("Found network: ");
        Serial.println(ssid);

        if (ssid == SECRET_HOME_SSID)
        {
            return NETWORK_HOME;
        }
        else if (ssid == SECRET_STUDIO_SSID)
        {
            return NETWORK_STUDIO;
        }
        else if (ssid == SECRET_UCB_SSID)
        {
            return NETWORK_CAMPUS;
        }
    }

    return NETWORK_UNKNOWN;
}

// Additional campus-specific authentication for UCB Wireless
bool handleUCBWirelessAuth()
{
    Serial.println("Using UCB Wireless specific authentication...");

    // Reset WiFi connection
    WiFi.disconnect();
    delay(1000);

    // Try to reduce power consumption during connection
    esp_wifi_set_ps(WIFI_PS_NONE);
    WiFi.setSleep(false);

    // Connect to the network
    WiFi.begin(SECRET_UCB_SSID);

    // Wait for initial connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nInitial connection established");

        // Add a short delay to stabilize connection before making HTTP requests
        delay(2000);

        // Try multiple authentication attempts with error handling
        try
        {
            // Create a secure client with server certificate validation disabled
            WiFiClientSecure client;
            client.setInsecure(); // Skip verification

            HTTPClient http;

            // Direct connection to the known UCB login URL
            const char *loginUrl = "https://nac.int.colorado.edu/captive-portal";
            http.begin(client, loginUrl);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

            // Add headers to mimic a browser
            http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
            http.addHeader("Host", "nac.int.colorado.edu");
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");

            // Construct POST data with username and password
            String postData = "username=" + urlEncode(identikey) +
                              "&password=" + urlEncode(password) +
                              "&submit=Sign+In";

            // Send POST request
            Serial.println("Sending direct authentication request to UCB portal...");
            int httpCode = http.POST(postData);

            Serial.print("Auth direct POST status: ");
            Serial.println(httpCode);

            if (httpCode > 0)
            {
                String response = http.getString();
                Serial.println("Response length: " + String(response.length()));
                Serial.println("First 100 chars: " + response.substring(0, 100));

                // Additional check to see if we're actually connected
                delay(5000); // Wait for authentication to complete

                if (WiFi.status() == WL_CONNECTED)
                {
                    // Try to access a website to confirm internet connectivity
                    http.end();
                    http.begin(client, "https://www.google.com");
                    int testCode = http.GET();

                    if (testCode == 200)
                    {
                        Serial.println("Successfully connected to the internet!");
                        http.end();

                        // Add a delay before returning to allow connection to stabilize
                        delay(3000);
                        return true;
                    }
                    else
                    {
                        Serial.println("Connected to WiFi but internet access may not be working");
                    }
                }
            }

            http.end();
        }
        catch (const std::exception &e)
        {
            Serial.print("Exception during campus auth: ");
            Serial.println(e.what());
        }
        catch (...)
        {
            Serial.println("Unknown exception during campus auth");
        }
    }

    Serial.println("UCB Wireless authentication failed");
    return false;
}